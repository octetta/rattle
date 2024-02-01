package main

// #cgo CFLAGS: -Isrc
// #cgo LDFLAGS: lib/librat.a lib/libamy.a lib/librma.a -lm
// #include "ratlib.h"
import "C"

import (
  "fmt"
  "time"
  "os"
  "embed"
  "github.com/pborman/getopt/v2"
  "github.com/chzyer/readline"
  "strings"
  "strconv"
  "io"
  "github.com/kerrigan29a/drawille-go"
  //"math"
  "path/filepath"
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

func samples() []int16 {
    var sample = make([]int16, frames())
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

func main() {
  pat := [][]string{
    []string{"v20l1","v20l0"},
    []string{"v30l0","v30l1"},
  }
  //palt := make([][]string, 10)
  ptr := []int{0,0}
  list := false
  getopt.Flag(&list, 'l', "list output devices")
  device := 0
  getopt.FlagLong(&device, "device", 'd', "device for output")
  var interval int64
  var latency int64
  interval = 250
  getopt.FlagLong(&interval, "interval", 'm', "millisecond between events")
  optHelp := getopt.BoolLong("help", 0, "help")
  getopt.Parse()

  if *optHelp {
    getopt.Usage()
    os.Exit(0)
  }

  if list {
		C.rat_list()
		os.Exit(0)
  }

	cache,_ := os.UserCacheDir()
	fmt.Println(string(cache))
  
	//sampleout := filepath.Join(cache, "sample.txt")

	//config := os.UserConfigDir()
	//fmt.Println(config)

	text,_ := folder.ReadFile("folder/sample.txt")
	fmt.Println(string(text))

  C.rat_device(C.int(device))

  C.rat_start()

    l,_ := readline.NewEx(&readline.Config{
      Prompt: "# ",
      HistoryFile: "/tmp/readline.tmp",
      InterruptPrompt: "^C",
    })
    defer l.Close()
    l.CaptureExitSignal()

    time.Sleep(time.Duration(interval) * time.Millisecond)

    var sample = make([]int16, 256)

    ticker := time.NewTicker(time.Duration(interval) * time.Millisecond)
    done := make(chan bool)
    var diff int64

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
        switch {
          case tok == "@":
            for i:=0; i<len(pat); i++ {
              for j:=0; j<len(pat[i]); j++ {
                fmt.Println(i,j,pat[i][j])
              }
            }
            
          case tok == "@0":
            if len(tok) > 1 {
              process("S20", now)
              process("S30", now)
              process("v20w7p45", now)
              process("v30w7p5", now)
              go func() {
                l := time.Now()
                for {
                  select {
                    case <- done:
                      return
                    case t := <- ticker.C:
                      diff = t.Sub(l).Milliseconds()
                      latency = diff
                      c := clk()
                      for i:=0; i<len(ptr); i++ {
                        p := ptr[i];
                        if p>=len(pat[i]) {
                          p = 0
                        }
                        process(pat[i][p], c)
                        ptr[i] = p+1
                      }
                      l = t
                  }
                }
              }()
            }
          case tok == "@1":
            done <- true
          case tok == "@2":
            pat[0] = append(pat[0], "v20l5")
            pat[0] = append(pat[0], "v20l0")
          case tok == "@3":
            pat[1] = append(pat[1], "v30l1")
            pat[1] = append(pat[1], "v30l0")
            pat[1] = append(pat[1], "v40l1")
            pat[1] = append(pat[1], "v40l0")
          case tok[:1] == ":":
            // : = system settings
            if len(tok) > 1 {
              switch {
                case tok[:2] == ":q":
                  goto exit
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
                      ticker.Reset(time.Duration(ms) * time.Millisecond)
                      interval = ms
                    }
                  } else {
                    fmt.Println(interval)
                  }
                case matcher(":[a-z]", tok):
                  fmt.Println("match :[a-z]")
                case matcher(":[a-z]=*", tok):
                  fmt.Println("match :[a-z]=*")
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
      }
    }
    exit:
    C.rat_stop()
    fmt.Println("done")
}
