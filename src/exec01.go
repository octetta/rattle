package main

import (
  "bufio"
  "fmt"
  "time"
  "os"
  "os/exec"
  "encoding/json"
  "embed"
  //"path/filepath"
  "github.com/pborman/getopt/v2"
)

func devices() {
    fmt.Println("> bin/rmini -l")
    cmd := exec.Command("./bin/rmini", "-l")
    out, _ := cmd.Output()
    type Result []interface{}
    var devices []Result 
    err := json.Unmarshal(out, &devices)
    if err != nil {
        fmt.Println("error:", err)
    }
    for i := 0; i < len(devices); i++ {
        fmt.Printf("%.0f \"%s\"\n", devices[i][0], devices[i][1])
    }
}

//go:embed folder/*
var folder embed.FS

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
		devices()
		os.Exit(0)
  }

	cache,_ := os.UserCacheDir()
	fmt.Println(string(cache))
	//sampleout := filepath.Join(cache, "sample.txt")

	//config := os.UserConfigDir()
	//fmt.Println(config)

	text,_ := folder.ReadFile("folder/sample.txt")
	fmt.Println(string(text))

    fmt.Printf("> bin/rmini -d %d\n", device)
    device_string := fmt.Sprintf("%d", device)
    exe := exec.Command("./bin/rmini", "-d", device_string)
    cmd,_ := exe.StdinPipe()
    res,_ := exe.StdoutPipe()
    // amyErr, _ := exe.StderrPipe()
    exe.Start()
    
    buf := bufio.NewReader(res)

    var clock int64
    var sample []int16

    cmd.Write([]byte("v0w8p100\n"))

    for i := 0; i < 3; i++ {
      cmd.Write([]byte("v0n40l1\n"))
      cmd.Write([]byte("?c\n"))
      line,_,_ := buf.ReadLine()

      json.Unmarshal(line, &clock)
      fmt.Println(clock)
      
      time.Sleep(time.Duration(interval) * time.Millisecond)
      time.Sleep(time.Duration(interval) * time.Millisecond)
      
      cmd.Write([]byte("v0n50l1\n"))
      cmd.Write([]byte("?c\n"))
      line,_,_ = buf.ReadLine()
      json.Unmarshal(line, &clock)
      fmt.Println(clock)
      
      time.Sleep(time.Duration(interval) * time.Millisecond)
      
      if i == 0 {
        cmd.Write([]byte("v0n60l1\n"))
        cmd.Write([]byte("<1024\n"))
        time.Sleep(time.Duration(interval) * time.Millisecond)
        cmd.Write([]byte("?i\n")) // get array [100,2] == 100 frames, 2frames/sample
        line,_,_ = buf.ReadLine()
        var info []int64
        json.Unmarshal(line, &info)
        fmt.Println(info)
        cmd.Write([]byte("?n\n")) // get array of frames [1,...]
        line,_,_ = buf.ReadLine()
        json.Unmarshal(line, &sample);
        fmt.Println(sample)
      } else {
        cmd.Write([]byte("v0n30l1\n"))
        time.Sleep(time.Duration(interval) * time.Millisecond)
      }

      
    }
    
    cmd.Close()
    res.Close()
    exe.Wait()

    // We ommited error checks in the above example, but
    // you could use the usual `if err != nil` pattern for
    // all of them. We also only collect the `StdoutPipe`
    // results, but you could collect the `StderrPipe` in
    // exactly the same way.

    // Note that when spawning commands we need to
    // provide an explicitly delineated command and
    // argument array, vs. being able to just pass in one
    // command-line string. If you want to spawn a full
    // command with a string, you can use `bash`'s `-c`
    // option:
    // lsCmd := exec.Command("bash", "-c", "ls -a -l -h")
    // lsOut, err := lsCmd.Output()
    // if err != nil {
    //     panic(err)
    // }
    // fmt.Println("> ls -a -l -h")
    // fmt.Println(string(lsOut))
}
