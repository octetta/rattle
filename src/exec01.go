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
  "io"
)

//go:embed folder/*
var folder embed.FS

func clk() int {
  var r C.uint = C.rat_clock()
  return int(r)
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
    fmt.Println("RATLIB!")
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

  C.rat_send(C.CString("v0w8p100"))

    for i := 0; i < 1; i++ {
      C.rat_send(C.CString("v0n40l1"))
      fmt.Println(clk())
      
      time.Sleep(time.Duration(interval) * time.Millisecond)
      time.Sleep(time.Duration(interval) * time.Millisecond)
      
      C.rat_send(C.CString("v0n50l1\n"))
      fmt.Println(clk())
      
      time.Sleep(time.Duration(interval) * time.Millisecond)
      
      if i == 0 {
        C.rat_send(C.CString("v0n60l1\n"))
        //cmd.Write([]byte("<1024\n"))
        time.Sleep(time.Duration(interval) * time.Millisecond)
        //cmd.Write([]byte("?i\n")) // get array [100,2] == 100 frames, 2frames/sample
        //line,_,_ = buf.ReadLine()
        //var info []int64
        //json.Unmarshal(line, &info)
        //fmt.Println(info)
        //cmd.Write([]byte("?n\n")) // get array of frames [1,...]
        //line,_,_ = buf.ReadLine()
        //json.Unmarshal(line, &sample);
        //fmt.Println(sample)
      } else {
        C.rat_send(C.CString("v0n30l1\n"))
        //time.Sleep(time.Duration(interval) * time.Millisecond)
      }

      
    }

    l,_ := readline.NewEx(&readline.Config{
      Prompt: "# ",
      HistoryFile: "/tmp/readline.tmp",
      InterruptPrompt: "^C",
    })
    defer l.Close()
    l.CaptureExitSignal()

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
      switch {
        case line == ":q":
          goto exit
        case line == "?c":
          fmt.Println(clk())
        case line == "?i":
          //x1 ,_,_ := buf.ReadLine()
          //fmt.Println(x1)
          //json.Unmarshal(x1, &i0)
        case line == "?n":
          //x2,_,_ := buf.ReadLine()
          //fmt.Println(string(x2))
          //json.Unmarshal(x2, &n0);
        default:
          //fmt.Println("<" + line + ">")
          //cmd.Write([]byte(line+"\n"))
          C.rat_send(C.CString(line))
      }
    }
    exit:
    C.rat_stop()
}
