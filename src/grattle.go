package main

// #cgo CFLAGS: -Isrc
// #cgo LDFLAGS: lib/librat.a lib/libamy.a lib/librma.a -lm
// #include "ratlib.h"
import "C"

import (
  "bufio"
  //"bytes"
  "embed"
  "fmt"
  "github.com/bit101/go-ansi"
  "github.com/chzyer/readline"
  "github.com/pborman/getopt/v2"
  "github.com/kerrigan29a/drawille-go"
  "io"
  //"math"
  "net"
  "os"
  "path/filepath"
  //"reflect"
  "regexp"
  "strconv"
  "strings"
  "time"
)

func udper(port int) {
  l := fmt.Sprintf("0.0.0.0:%d", port)
  addr,err := net.ResolveUDPAddr("udp", l)
  if err != nil {
    fmt.Println(err)
    return
  }
  con,err := net.ListenUDP("udp", addr)
  if err != nil {
    fmt.Println(err)
    return
  }
  for {
    var buf [512]byte
    //n,addr,err := con.ReadFromUDP(buf[0:])
    n,_,err := con.ReadFromUDP(buf[0:])
    if err != nil {
      fmt.Println(err)
    } else {
      s := strings.TrimSpace(string(buf[0:n-1]))
      toker(s, clk())
    }
  }
}

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
  if debug {
    fmt.Println("=> framer", n)
  }
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
  if debug {
    fmt.Println("=>", line)
  }
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
  switch {
    case line[:1] == "-":
      amy(line[1:])
    case line[:1] == "+":
      m1 := re3.FindStringSubmatch(line)
      if len(m1) > 0 {
        ms := re3.SubexpIndex("Ms")
        rest := re3.SubexpIndex("Rest")
        if len(m1[rest]) > 0 {
          n,_ := strconv.Atoi(m1[ms])
          //later(m1[rest], n)
          s := fmt.Sprintf("t%d%s", n+now, m1[rest])
          amy(s)
        }
      }
    case line[:1] == "_":
      m1 := re4.FindStringSubmatch(line)
      if len(m1) > 0 {
        top := re4.SubexpIndex("Top")
        bot := re4.SubexpIndex("Bot")
        rest := re4.SubexpIndex("Rest")
        if len(m1[rest]) > 0 {
          i,_ := strconv.Atoi(m1[top])
          i--
          j,_ := strconv.Atoi(m1[bot])
          j--
          //later(m1[rest], meter[i][j])
          s := fmt.Sprintf("t%d%s", now+meter[i][j], m1[rest])
          amy(s)
        }
      }
    case line[:1] == "t":
      amy(line)
    default:
      s := fmt.Sprintf("t%d%s", now, line)
      amy(s)
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
  s.Clear()
  for i := 0; i < l; i+=2 {
    x0 := xlate(i, 0, l-1, 0, tx*2-2)
    y0 := xlate(int(a[i]), int(lo), int(hi), 0, ty/2)
    s.Set(x0, y0)
  }
  ansi.SetFg(ansi.Green)
  fmt.Print(s)
  ansi.ResetAll()
  ansi.SetFg(ansi.Red)
  fmt.Printf("min:%d max:%d range:%d duration:%gms\n", lo, hi, ht,
    float64(frames()) / 2.0 / 44.1)
  s.Clear()
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
  fmt.Printf("# %d len:%d ptr:%d run:%d +%d mod:%d\n", n, c, ptr[n], run[n], mark[n], mod[n])
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
var debug bool
var dlevel int
var port int

var done chan bool
var metro chan int

func later(cmd string, ms int) {
  var t *time.Timer
  f := func() {
    //fmt.Println("--> #2", clk(), cmd)
    amy(cmd)
    t.Stop()
  }
  d := time.Duration(ms) * time.Millisecond
  t = time.AfterFunc(d, f)
}

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
  for _, v := range all {
    r = _toker(v, now)
  }
  // return the last value in the sequence
  return r
}

func help(n int) {
  fmt.Println("### AMY wire protocol")
  fmt.Println("a amp without retrigger [0,1.0)         | A bp0 time,ratio,...up-to-8-pairs...")
  fmt.Println("b algo feedback [0,1.0]                 | B bp1")
  fmt.Println("c chained osc                           | C clone osc")
  fmt.Println("d duty cycle of pulse [0.01,0.99]       | D debug")
  fmt.Println("f frequency of osc                      | F center freq for filter")
  fmt.Println("g modulation tarGet                     | G filter 0=none 1=LPF 2=BPF 3=HP")
  fmt.Println("  1=amp         2=duty  4=freq          | I ratio (see AMY webpage)")
  fmt.Println("  8=filter-freq 16=rez  32=feedback     | L mod source osc / LFO")
  fmt.Println("l velocity/trigger >0.0 or note-off =0  | N latency in ms")
  fmt.Println("n midi note number                      | O oscs algo 6 comma-sep, -1=none")
  fmt.Println("o DX7 algorithm [1,32]                  | P phase [0,1.0] where osc cycle ")
  fmt.Println("p patch                                 | R rez q filter [0,10.0], defaultstarts")
  fmt.Println("t timestamp in ms for event             | S reset oscs")
  fmt.Println("v osc selector [0,63]                   | T bp0 (A) target")
  fmt.Println("w wave type                             | W bp1 (B) target")
  fmt.Println("  0=sine 1=pulse  2=saw-dn 3=saw-up     | X bp2 (c) target")
  fmt.Println("  4=tri  5=noise  6=ks     7=pcm        | V volume knob for everything")
  fmt.Println("  8=algo 9=part  10=parts 11=off        |")
  fmt.Println("x eq_l in dB f[center]=800Hz [-15.0,15.0] 0=off")
  fmt.Println("y eq_m in dB f[center]=2500Hz      \"")
  fmt.Println("z eq_h in dB f[center]=7500Hz      \"")
  fmt.Println("(chorus) k level [0-1.0)    | m delay [1-512]")
  fmt.Println("(reverb) H liveness [0-1.0] | h level [0.0-inf)")
  fmt.Println("(reverb) j decay [0-1.0]    | J crossover Hz (3000)")

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
          case tok == ":d=0":
            debug = false
          case tok == ":d=1":
            debug = true
            dlevel = 1
          case tok == ":d=2":
            debug = true
            dlevel = 2
          case tok == ":d=3":
            debug = true
            dlevel = 3
          case tok[:2] == ":q":
            return -1
          case tok[:2] == ":b":
            fmt.Println(int(C.rat_block_size()))
          case tok[:2] == ":c":
            ansi.ClearScreen()
          case tok[:2] == ":r":
            fmt.Println(int(C.rat_sample_rate()))
          case tok[:2] == ":s":
            C.rat_see()
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
            //fmt.Println("b =>", b)
            if b[0] == "+" {
              m := mark[n]
              s := strings.ReplaceAll(b[1], "&", ";")
              //fmt.Println("ADD @", m, b[1])
              pat[n][m] = s
              mark[n] = m+1
            } else {
              m, _ := strconv.ParseInt(b[0], 10, 32)
              if m >= 0 && m < int64(len(pat[n])) {
                //pat[n][m] = b[1]
                s := strings.ReplaceAll(b[1], "&", ";")
                pat[n][m] = s
                mark[n] = int(m)+1
                //fmt.Println("MANUAL @", m, s)
              }
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
    case tok == "?":
      help(0)
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
          if tok[1:] == "g" {
            C.rat_global_dump()
          } else if tok[1:] == "a" {
            C.rat_osc_multi(C.int(1))
          } else if tok[1:] == "f" {
            C.rat_osc_multi(C.int(0))
          } else if tok[1:] == "r" {
            C.rat_r2v()
          } else if tok[1:] == "v" {
            C.rat_v2r()
          } else {
            n,_ := strconv.Atoi(tok[1:])
            C.rat_osc_dump(C.int(n))
          }
      }
    case tok[:1] == "~":
      // ~ = pause
      if len(tok) > 1 {
        ms, _ := strconv.ParseInt(tok[1:], 10, 32)
        time.Sleep(time.Duration(ms) * time.Millisecond)
      } else {
        time.Sleep(time.Duration(interval) * time.Millisecond)
      }
    case tok[:1] == ">":
      if len(tok) > 1 {
        n := len(tok)
        if tok[n-1:n] == "<" {
          p, _ := strconv.ParseInt(tok[1:n-1], 10, 32)
          fmt.Println(">", p, "<")
          C.rat_patch_from_framer(C.int(p))
        } else {
          p, _ := strconv.ParseInt(tok[1:], 10, 32)
          C.rat_patch_show(C.int(p))
        }
      }
    case tok[:1] == "<":
      // < = capture frames
      if len(tok) > 1 {
        ms, _ := strconv.ParseInt(tok[1:], 10, 32)
        n := (ms * 44100) / 1000 * 2
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
      process(tok, clk())
    case tok[:1] == "-":
      amy(tok[1:])
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
var mark []int

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
  mark = make([]int, 10)
  for i:=0; i<len(ptr); i++ {
    ptr[i] = 0
    run[i] = 0
    mod[i] = 4
    mark[i] = 0
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

  port = 8405
  getopt.FlagLong(&port, "port", 'u', "port for UDP listner")

  debug = false
  getopt.Flag(&debug, 'D', "enable debug messages")
  
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

	//cache,_ := os.UserCacheDir()
	//fmt.Println(string(cache))
  
	//sampleout := filepath.Join(cache, "sample.txt")

	//config := os.UserConfigDir()
	//fmt.Println(config)

	//text,_ := folder.ReadFile("folder/sample.txt")
	//fmt.Println(string(text))

  C.rat_device(C.int(device))

  C.rat_start()

  done = make(chan bool)
  metro = make(chan int)

  amy("N1"); // this might fix the weird latency stuff?

  go runner()

  go udper(port)

  if usefile != "" {
    file, err := os.Open(usefile)
    if err != nil {
      fmt.Println("can not open", usefile)
    } else {
      scanner := bufio.NewScanner(file)
      now := clk()
      for scanner.Scan() {
        //fmt.Println(scanner.Text())
        toker(scanner.Text(), now)
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
    if debug {
      fmt.Println("# clk", clk())
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
