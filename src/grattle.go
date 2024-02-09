package main

// #cgo CFLAGS: -Isrc
// #cgo LDFLAGS: lib/librat.a lib/libamy.a lib/librma.a -lm
// #include "ratlib.h"
import "C"

import (
  "bufio"
  "embed"
  "fmt"
  "github.com/bit101/go-ansi"
  "github.com/chzyer/readline"
  "github.com/pborman/getopt/v2"
  "github.com/kerrigan29a/drawille-go"
  "io"
  //"math"
  "os"
  "path/filepath"
  //"reflect"
  "regexp"
  "strconv"
  "strings"
  "time"
)

func matcher(p string, s string) bool {
  f,_ := filepath.Match(p, s)
  return f
}

//go:embed folder/*
var folder embed.FS

func clk() int {
  var r C.uint = C.rat_clock()
  return int(r)
}

func frames() int {
  var r C.int = C.rat_frames()
  return int(r)
}

func framer(n int) {
  C.rat_framer(C.int(n))
}

var sample []int16

func samples() []int16 {
    sample = make([]int16, frames())
    for i, _ := range sample {
      sample[i] = int16(C.rat_frame_at(C.int(i)))
    }
    return sample
}

func amy(line string) {
    C.rat_send(C.CString(line))
}

var re1 *regexp.Regexp
var re2 *regexp.Regexp
var re3 *regexp.Regexp
var re4 *regexp.Regexp

func process(line string, now int) {
  // expand $ value
  b := strings.Split(line, "$")
  if len(b) > 1 {
    changed := false
    for i:=1; i<len(b); i++ {
      if len(b[i]) > 0 && b[i][0] >= 'a' && b[i][0] <= 'z' {
        n := b[i][0] - 97
        c := usr[n] + b[i][1:]
        b[i] = c
        changed = true
      } else {
      }
    }
    if changed {
      line = strings.Join(b, "")
    }
  }
  snow := strconv.FormatInt(int64(now), 10)
  switch {
    case line[:1] == "+":
      m1 := re3.FindStringSubmatch(line)
      if len(m1) > 0 {
        ms := re3.SubexpIndex("Ms")
        rest := re3.SubexpIndex("Rest")
        //fmt.Println(m1[ms], m1[rest])
        if len(m1[rest]) > 0 {
          n,_ := strconv.Atoi(m1[ms])
          s := fmt.Sprintf("t%d%s", n+now, m1[rest])
          //fmt.Println("=> ", s)
          amy(s)
        }
      }
    case line[:1] == "_":
      m1 := re4.FindStringSubmatch(line)
      if len(m1) > 0 {
        //fmt.Println("m1 -> ", m1)
        top := re4.SubexpIndex("Top")
        bot := re4.SubexpIndex("Bot")
        rest := re4.SubexpIndex("Rest")
        if len(m1[rest]) > 0 {
          //fmt.Println(" top -> ", m1[top])
          //fmt.Println(" bot -> ", m1[bot])
          //fmt.Println("rest -> ", m1[rest])
          i,_ := strconv.Atoi(m1[top])
          i--
          j,_ := strconv.Atoi(m1[bot])
          j--
          s := fmt.Sprintf("t%d%s", meter[i][j]+now, m1[rest])
          //fmt.Println("=> ", s)
          amy(s)
        }
      }
    case line[:1] == "t":
      amy(line)
    default:
      out := "t" + snow + line
      //fmt.Println(out)
      amy(out)
  }
}

func xlate(x int, a int, b int, c int, d int) int {
  if (b-a) == 0 {
    return 0
  }
  xp := (x-a) * (d-c)/(b-a)+c
  return xp
}

func graph(a []int16) {
  if len(a) == 0 {
    return
  }
  s := drawille.NewCanvas()
  //for x := 0; x < (900); x = x + 1 {
  //  y := int(math.Sin((math.Pi/180)*float64(x))*10 + 0.5)
  //  s.Set(x/10, y)
  //}
  //fmt.Print(s)
  //s.Clear()
  r := len(a) / 160
  if r <= 0 {
    r = 1
  }
  hi := a[0]
  lo := a[0]
  l := len(a)
  for i := 1; i < l; i++ {
    if a[i] > hi {
      hi = a[i]
    }
    if a[i] < lo {
      lo = a[i]
    }
  }
  ht := hi-lo
  if ht < 0 {
    ht = -ht
  }
  tx,ty := ansi.TermSize()
  for i := 0; i < l; i+=2 {
    x0 := xlate(i, 0, l-1, 0, tx*2-2)
    y0 := xlate(int(a[i]), int(lo), int(hi), 0, ty/2)
    s.Set(x0, y0)
  }
  ansi.SetFg(ansi.Green)
  fmt.Print(s)
  ansi.ResetAll()
  ansi.SetFg(ansi.Red)
  fmt.Println(lo, hi, ht)
  for i := 1; i < l; i+=2 {
    x0 := xlate(i, 0, l-1, 0, tx*2-2)
    y0 := xlate(int(a[i]), int(lo), int(hi), 0, ty/2)
    s.Set(x0, y0)
  }
  ansi.SetFg(ansi.Purple)
  fmt.Print(s)
  ansi.ResetAll()
}

func _dump(n int, all bool) {
  c := 0
  one := pat[n]
  for i:=0; i<len(one); i++ {
    if len(one[i]) == 0 {
      break
    }
    c++
  }
  if c == 0 {
    return
  }
  fmt.Printf("# %d len:%d ptr:%d run:%d mod:%d\n", n, c, ptr[n], run[n], mod[n])
  if all {
  for i:=0; i<len(one); i++ {
    if len(one[i]) == 0 {
      break
    }
    fmt.Printf(":%d/%d=%s\n",n,i,one[i])
  }
  }
}

func dump() {
  for i:=0; i<len(pat); i++ {
    _dump(i, false)
  }
}

var interval int
var latency int64

var done chan bool
var metro chan int

func runner() {
  l := time.Now()
  interval = 250
  pulse := 0
  ticker := time.NewTicker(time.Duration(interval) * time.Millisecond)
  for {
    select {
      case <- done:
        return
      case interval = <- metro:
        ticker.Reset(time.Duration(interval) * time.Millisecond)
      case t := <- ticker.C:
        diff := t.Sub(l).Milliseconds()
        latency = diff
        c := clk()
        for i:=0; i<len(ptr); i++ {
          if run[i] == 0 {
            continue
          }
          if (pulse % mod[i]) == 0 {
            // execute step
            p := ptr[i];
            if p>=len(pat[i]) {
              p = 0
            }
            if pat[i][p] == "/" {
              p = 0
            }
            if len(pat[i][p]) > 0 {
              //process(pat[i][p], c)
              toker(pat[i][p], c)
              ptr[i] = p+1
            } else {
              run[i] = 0
              ptr[i] = 0
            }
            //
          }
        }
        l = t
        pulse++
    }
  }
}

func toker(tok string, now int) int {
  all := strings.Split(tok, ";")
  var r = 0
  if len(all) > 1 {
    fmt.Println(all)
  }
  for _, v := range all {
    r = _toker(v, now)
  }
  // return the last value in the sequence
  return r
}

func _toker(tok string, now int) int {
  switch {
    case tok == "":
      return 0
    case tok[:1] == "#":
      return 0
    case tok == "::":
      dump()
    case tok[:1] == ":":
      // : = system settings
      if len(tok) > 1 {
        switch {
          case tok[:2] == ":q":
            return -1
          case tok[:2] == ":b":
            fmt.Println(int(C.rat_block_size()))
          case tok[:2] == ":c":
            ansi.ClearScreen()
          case tok[:2] == ":r":
            fmt.Println(int(C.rat_sample_rate()))
          case tok[:2] == ":o":
            fmt.Println(int(C.rat_oscs()))
          case tok[:2] == ":m":
            if len(tok) > 2 {
              ms, err := strconv.ParseInt(tok[3:], 10, 32)
              if err == nil {
                interval = int(ms)
                metro <- int(interval)
                //
                for i:=0; i<8; i++ {
                  meter[i] = make([]int, 8)
                  for j:=0; j<8; j++ {
                    meter[i][j] = (interval * 4) * (i+1) / (j+1)
                  }
                }
                //
              }
            } else {
              fmt.Println(interval)
            }
          case matcher(":[0-9]", tok):
            n := int(tok[1]-48)
            _dump(n, true)
          case matcher(":[0-9]/[prs]", tok):
            n := int(tok[1]-48)
            a := tok[3:]
            switch {
              case a == "p":
                //fmt.Println("pause")
                run[n] = 0
              case a == "r":
                //fmt.Println("resume")
                run[n] = 1
              case a == "s":
                //fmt.Println("stop")
                run[n] = 0
                ptr[n] = 0
            }
          case re1.MatchString(tok):
            n := int(tok[1]-48)
            arg := tok[3:]
            b := strings.SplitN(arg, "=", 2)
            m, _ := strconv.ParseInt(b[0], 10, 32)
            if m >= 0 && m < int64(len(pat[n])) {
              //pat[n][m] = b[1]
              s := strings.ReplaceAll(b[1], "&", ";")
              pat[n][m] = s
            }
          case re2.MatchString(tok):
            // %4=5
            // 0123
            n := int(tok[1]-48)
            arg := tok[3:]
            m, _ := strconv.ParseInt(arg, 10, 32)
            if m >= 0 && m < int64(len(mod)) {
              mod[n] = int(m)
            }
          case matcher(":[a-z]", tok):
            fmt.Println("match :[a-z]")
          case matcher(":[a-z]=*", tok):
            fmt.Println("match :[a-z]=*")
        }
      }
    case tok[:1] == "%":
      if len(tok) == 1 {
        for i:=0; i<len(mod); i++ {
          fmt.Printf("%%%d=%d\n", i, mod[i])
        }
      } else if len(tok) == 2 {
        n := int(tok[1] - 48)
        fmt.Printf("%%%d=%d\n", n, mod[n])
      } else {
        if re2.MatchString(tok) {
          // %4=5
          // 0123
          n := int(tok[1]-48)
          arg := tok[3:]
          m, _ := strconv.ParseInt(arg, 10, 32)
          if n >= 0 && n < len(mod) {
            mod[n] = int(m)
          }
        } else {
          fmt.Println("% i do not understand")
        }
      }
    case tok == "?p":
      graph(samples())
    case tok == "?i":
      fmt.Println(frames())
    case tok == "?n":
      sample = samples()
      fmt.Println(sample)
    case tok == "?c":
      fmt.Println(clk())
    case tok == "?l":
      fmt.Println(latency)
    case tok[:1] == "@":
      // @ = amy example
      if len(tok) > 1 {
        ex, _ := strconv.ParseInt(tok[1:], 10, 32)
        C.rat_example(C.int(ex))
      }
    case tok[:1] == "~":
      // ~ = pause
      if len(tok) > 1 {
        ms, _ := strconv.ParseInt(tok[1:], 10, 32)
        time.Sleep(time.Duration(ms) * time.Millisecond)
      } else {
        time.Sleep(time.Duration(interval) * time.Millisecond)
      }
    case tok[:1] == "<":
      // < = capture frames
      if len(tok) > 1 {
        ms, _ := strconv.ParseInt(tok[1:], 10, 32)
        n := (ms * 44100) / 1000
        framer(int(n))
      } else {
        framer(44100 * 2)
      }
    case tok[:1] == "/":
      fmt.Println("/ = jump")
    case tok[:1] == "$":
      if len(tok) == 1 {
        for i:=0; i<VLEN; i++ {
          if len(usr[i]) > 0 {
            fmt.Printf("$%s=%s\n", key[i], usr[i])
          }
        }
      } else if len(tok) == 2 {
        n := int(tok[1]-97)
        if (n >= 0) && (n <= 26) {
          fmt.Println(usr[n])
        }
      } else if len(tok) > 2 {
        n := int(tok[1]-97)
        // $a=he
        // 0123
        if (n >= 0) && (n <= 26) {
          usr[n] = tok[3:]
        }
      } else {
        fmt.Println("$ ???")
      }
    case matcher("[a-zA-Z]*", tok):
      //amy(tok)
      process(tok, clk())
    case tok[:1] == "+":
      process(tok, clk())
    case tok[:1] == "_":
      if len(tok) == 1 {
        // show whole table
        for i:=0; i<8; i++ {
          for j:=0; j<8; j++ {
            fmt.Printf("_%d%d %-5d  ", i+1, j+1, meter[i][j])
          }
          fmt.Println("")
        }
      } else if len(tok) == 2 {
        // show one row
        fmt.Println("row --")
        i := int(tok[1]-48)
        i--
        for j:=0; j<8; j++ {
          fmt.Printf("_%d%d %-5d  ", i+1, j+1, meter[i][j])
        }
        fmt.Println("")
      } else if len(tok) == 3 {
        fmt.Println("one --")
        // show one entry
        i := int(tok[1]-48)
        i--
        j := int(tok[2]-48)
        j--
        fmt.Printf("_%d%d %-5d\n", i+1, j+1, meter[i][j])
      } else {
        process(tok, clk())
      }
    default:
      fmt.Println("???", tok)
  }
  return 0
}

var pat [][]string
var ptr []int
var mod []int
var run []int
var usr []string
var key []string
var meter [][]int

var PLEN int
var VLEN int

func main() {
  PLEN = 100
  VLEN = 26
  pat = make([][]string, 10)
  ptr = make([]int, 10)
  mod = make([]int, 10)
  run = make([]int, 10)
  for i:=0; i<len(ptr); i++ {
    ptr[i] = 0
    run[i] = 0
    mod[i] = 4
    pat[i] = make([]string, PLEN)
    for j:=0; j<len(pat[i]); j++ {
      pat[i][j] = ""
    }
  }
  
  usr = make([]string, VLEN)
  key = make([]string, VLEN)
  for i:=0; i<VLEN; i++ {
    usr[i] = ""
    key[i] = string(i+97)
  }

  meter = make([][]int, 8)
  
  list := false
  getopt.Flag(&list, 'l', "list output devices")
  
  device := 0
  getopt.FlagLong(&device, "device", 'd', "device for output")

  interval = 250
  getopt.FlagLong(&interval, "interval", 'm', "millisecond between events")
  
  optHelp := getopt.BoolLong("help", 0, "help")
  
  usefile := ""
  getopt.FlagLong(&usefile, "file", 'f', "read list of rattle commands")
  
  getopt.Parse()

  for i:=0; i<8; i++ {
    meter[i] = make([]int, 8)
    for j:=0; j<8; j++ {
      meter[i][j] = (interval * 4) * (i+1) / (j+1)
    }
  }
  
  if *optHelp {
    getopt.Usage()
    os.Exit(0)
  }

  if list {
		C.rat_list()
		os.Exit(0)
  }

  re1 = regexp.MustCompile(`:[0-9]/*`)
  re2 = regexp.MustCompile(`%[0-9]=*`)
  re3 = regexp.MustCompile(`\+(?P<Ms>\d+)(?P<Rest>.*)`)
  re4 = regexp.MustCompile(`_(?P<Top>\d)(?P<Bot>\d)(?P<Rest>.*)`)

	cache,_ := os.UserCacheDir()
	fmt.Println(string(cache))
  
	//sampleout := filepath.Join(cache, "sample.txt")

	//config := os.UserConfigDir()
	//fmt.Println(config)

	text,_ := folder.ReadFile("folder/sample.txt")
	fmt.Println(string(text))

  C.rat_device(C.int(device))

  C.rat_start()

  done = make(chan bool)
  metro = make(chan int)

  go runner()

  if usefile != "" {
    file, err := os.Open(usefile)
    if err != nil {
      fmt.Println("can not open ", usefile)
    } else {
      scanner := bufio.NewScanner(file)
      now := clk()
      for scanner.Scan() {
        toker(scanner.Text(), now)
        //fmt.Println(scanner.Text())
      }
      file.Close()
    }
  }

  l,_ := readline.NewEx(&readline.Config{
    Prompt: "# ",
    HistoryFile: "/tmp/readline.tmp",
    InterruptPrompt: "^C",
  })
  defer l.Close()
  l.CaptureExitSignal()

  time.Sleep(time.Duration(interval) * time.Millisecond)

  for {
    line,err := l.Readline()
    if err == readline.ErrInterrupt {
      if len(line) == 0 {
        break
      } else {
        continue
      }
    } else if err == io.EOF {
      break
    }
    line = strings.TrimSpace(line)
    if len(line) == 0 {
      continue
    }
    a := strings.Split(line, ";")
    now := clk()
    for _, ptok := range a {
      tok := strings.TrimSpace(ptok)
      if len(tok) == 0 {
        continue
      }
      if toker(tok, now) < 0 {
        goto exit
      }
    }
  }
  exit:
  done <- true
  C.rat_stop()
  fmt.Println("done")
}
