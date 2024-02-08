package main

// #cgo CFLAGS: -Isrc
// #cgo LDFLAGS: lib/librat.a lib/libamy.a lib/librma.a -lm
// #include "ratlib.h"
import "C"

import (
  "bufio"
  "embed"
  "fmt"
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

func process(line string, now int) {
  // expand $ value
  snow := strconv.FormatInt(int64(now), 10)
  switch {
    case line[:1] == "+":
      // expand + value
      // get number following and add to now
      fmt.Println("+")
    case line[:1] == "_":
      // expand _ value
      // get two digits following, look up in meter and add to now
      fmt.Println("_")
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
  for i := 0; i < l; i+=2 {
    x0 := xlate(i, 0, l-1, 0, 160)
    y0 := xlate(int(a[i]), int(lo), int(hi), 0, 40)
    s.Set(x0, y0)
  }
  fmt.Print(s)
  fmt.Println(lo, hi, ht)
  for i := 1; i < l; i+=2 {
    x0 := xlate(i, 0, l-1, 0, 160)
    y0 := xlate(int(a[i]), int(lo), int(hi), 0, 40)
    s.Set(x0, y0)
  }
  fmt.Print(s)
}

func _dump(n int) {
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
  for i:=0; i<len(one); i++ {
    if len(one[i]) == 0 {
      break
    }
    fmt.Printf(":%d/%d=%s\n",n,i,one[i])
  }
}

func dump() {
  for i:=0; i<len(pat); i++ {
    _dump(i)
  }
}

var interval int64
var latency int64

var done chan bool
var metro chan int

func runner() {
  l := time.Now()
  interval := 250
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
              process(pat[i][p], c)
              ptr[i] = p+1
            }
            //
          }
        }
        l = t
        pulse++
    }
  }
}

var re1 *regexp.Regexp
var re2 *regexp.Regexp

func toker(tok string, now int) int {
  //fmt.Println("# got ", tok, now)
  switch {
    case tok == "":
      return 0
    case tok[:1] == "#":
      return 0
    //case tok == "@0":
    //  if len(tok) > 1 {
    //    go runner()
    //  }
    //case tok == "@1":
    //  done <- true
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
          case tok[:2] == ":r":
            fmt.Println(int(C.rat_sample_rate()))
          case tok[:2] == ":o":
            fmt.Println(int(C.rat_oscs()))
          case tok[:2] == ":m":
            if len(tok) > 2 {
              ms, err := strconv.ParseInt(tok[3:], 10, 64)
              if err == nil {
                interval = ms
                fmt.Println("# sending new metro time", interval)
                metro <- int(interval)
                fmt.Println("# after metro send")
              }
            } else {
              fmt.Println(interval)
            }
          case matcher(":[0-9]", tok):
            n := int(tok[1]-48)
            _dump(n)
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
            b := strings.Split(arg, "=")
            m, _ := strconv.ParseInt(b[0], 10, 32)
            if m >= 0 && m < int64(len(pat[n])) {
              pat[n][m] = b[1]
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
      } else {
        if re2.MatchString(tok) {
          // %4=5
          // 0123
          n := int(tok[1]-48)
          arg := tok[3:]
          m, _ := strconv.ParseInt(arg, 10, 32)
          if m >= 0 && m < int64(len(mod)) {
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
      fmt.Println("$ = var")
    case matcher("[a-zA-Z]*", tok):
      //amy(tok)
      process(tok, clk())
    default:
      fmt.Println("???", tok)
  }
  return 0
}

var pat [][]string
var ptr []int
var mod []int
var run []int

func main() {
  PLEN := 100
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
        fmt.Println(scanner.Text())
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

  //re1 := regexp.MustCompile(`:[0-9]/*`)
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
