package main

import "fmt"
import "time"
import "io/ioutil"
import "os/exec"
//import "encoding/json"

func main() {
    listCmd := exec.Command("./bin/rmini", "-l")
    listOut, err := listCmd.Output()
    if err != nil {
        panic(err)
    }
    fmt.Println("> bin/rmini -l")
    fmt.Println(string(listOut))

    //type Device struct {
    //  Id int
    //  Name string
    //}

    //var devices []Device
    //e := json.Unmarshal(listOut, &devices)
    //if e != nil {
    //  fmt.Println("error:", e)
    //}
    //fmt.Printf("%+v", devices)

    // `.Output` is another helper that handles the common
    // case of running a command, waiting for it to finish,
    // and collecting its output. If there were no errors,
    // `dateOut` will hold bytes with the date info.

    // Next we'll look at a slightly more involved case
    // where we pipe data to the external process on its
    // `stdin` and collect the results from its `stdout`.
    amyExec := exec.Command("./bin/rmini", "-d", "11")

    // Here we explicitly grab input/output pipes, start
    // the process, write some input to it, read the
    // resulting output, and finally wait for the process
    // to exit.
    amyCmd, _ := amyExec.StdinPipe()
    amyRes, _ := amyExec.StdoutPipe()
    // amyErr, _ := amyExec.StderrPipe()
    amyExec.Start()
    amyCmd.Write([]byte("?c\n"))
    amyCmd.Write([]byte("v0w1f110l1\n"))
    amyCmd.Write([]byte("?c\n"))
    time.Sleep(1 * time.Second)
    amyCmd.Write([]byte("v0f55\n"))
    amyCmd.Write([]byte("?c\n"))
    time.Sleep(1 * time.Second)
    amyCmd.Write([]byte("v0f5\n"))
    amyCmd.Write([]byte("?c\n"))
    time.Sleep(1 * time.Second)
    amyCmd.Write([]byte("<256\n"))
    //x,_ := ioutil.ReadAll(amyRes)
    //fmt.Println(string(x))
    time.Sleep(1 * time.Second)
    amyCmd.Write([]byte("?i\n")) // get array [100,2] == 100 frames, 2frames/sample
    amyCmd.Write([]byte("?n\n")) // get array of frames [1,...]
    //var sample []int16;
    //json.
    amyCmd.Close()
    amyBytes, _ := ioutil.ReadAll(amyRes)
    amyExec.Wait()

    // We ommited error checks in the above example, but
    // you could use the usual `if err != nil` pattern for
    // all of them. We also only collect the `StdoutPipe`
    // results, but you could collect the `StderrPipe` in
    // exactly the same way.
    fmt.Println("> bin/rmini -d 11")
    fmt.Println(string(amyBytes))

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
