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
)

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
    default:
      out := "t" + snow + line
      fmt.Println(out)
      amy(out)
  }
}

func graph(a []int16) {
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
  //s.Set(0, -32000)
  //s.Set(1, 32000)
  hi := a[0]
  lo := a[0]
  for i := 1; i < len(a); i+=2 {
    if a[i] > hi {
      hi = a[i]
    }
    if a[i] < lo {
      lo = a[i]
    }
    x0 := int(i/r)
    //m := (a[i] + a[i+1]) / 2
    m := a[i] / 64
    y0 := int(m)
    s.Set(x0, y0)
  }
  fmt.Print(s)
  fmt.Println(lo, hi)
}

func main() {
  list := false
  getopt.Flag(&list, 'l', "list output devices")
  device := 0
  getopt.FlagLong(&device, "device", 'd', "device for output")
  interval := 250
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
      for _, ptok := range a {
        tok := strings.TrimSpace(ptok)
        if len(tok) == 0 {
          continue
        }
        switch {
          case tok[:1] == ":":
            // : = system settings
            if len(tok) > 1 {
              switch {
                case tok[1] == 'q':
                  goto exit
              }
            }
          case tok[:1] == "?":
            // ? = query
            if len(tok) > 1 {
              switch {
                case tok[1] == 'p':
                  graph(samples())
                case tok[1] == 'i':
                  fmt.Println(frames())
                case tok[1] == 'n':
                  sample = samples()
                  fmt.Println(sample)
                case tok[1] == 'c':
                  fmt.Println(clk())
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
          default:
            //amy(tok)
            process(tok, clk())
        }
      }
    }
    exit:
    C.rat_stop()
    fmt.Println("done")
}
