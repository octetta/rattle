package main

import (
  "bufio"
  "fmt"
  "time"
  "os"
  "os/exec"
  "encoding/json"
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

func main() {
	arg := os.Args
	fmt.Println(len(arg), arg)

	devid := "0"

	if len(arg) > 1 && arg[1] == "-l" {
		devices()
		os.Exit(0)
	}

	if len(arg) > 2 && arg[1] == "-d" {
		devid = arg[2]
	}

    fmt.Printf("> bin/rmini -d %s\n", devid)
    exe := exec.Command("./bin/rmini", "-d", devid)
    cmd,_ := exe.StdinPipe()
    res,_ := exe.StdoutPipe()
    // amyErr, _ := exe.StderrPipe()
    exe.Start()
    
    buf := bufio.NewReader(res)

    var clock int64
    var sample []int16

    cmd.Write([]byte("v0w1\n"))

    for i := 0; i < 3; i++ {
    
    cmd.Write([]byte("v0f110l1\n"))
    cmd.Write([]byte("?c\n"))
    line,_,_ := buf.ReadLine()

    json.Unmarshal(line, &clock)
    fmt.Println(clock)
    
    time.Sleep(250 * time.Millisecond)
    
    cmd.Write([]byte("v0f55\n"))
    cmd.Write([]byte("?c\n"))
    line,_,_ = buf.ReadLine()
    json.Unmarshal(line, &clock)
    fmt.Println(clock)
    
    time.Sleep(250 * time.Millisecond)
    
    cmd.Write([]byte("v0f5\n"))
    cmd.Write([]byte("?c\n"))
    line,_,_ = buf.ReadLine()
    json.Unmarshal(line, &clock)
    fmt.Println(clock)
    
    time.Sleep(250 * time.Millisecond)
    
    cmd.Write([]byte("<256\n"))
    
    time.Sleep(250 * time.Millisecond)
    
    cmd.Write([]byte("?i\n")) // get array [100,2] == 100 frames, 2frames/sample
    line,_,_ = buf.ReadLine()

    var info []int64

    json.Unmarshal(line, &info)
    fmt.Println(info)
    
    cmd.Write([]byte("?n\n")) // get array of frames [1,...]
    line,_,_ = buf.ReadLine()
    json.Unmarshal(line, &sample);
    fmt.Println(sample)

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
