package main

import "fmt"
import "time"
import "io/ioutil"
import "os/exec"

func main() {
    listCmd := exec.Command("./bin/rmini", "-l")
    listOut, err := listCmd.Output()
    if err != nil {
        panic(err)
    }
    fmt.Println("> bin/rmini -l")

    fmt.Println(string(listOut))
    // `.Output` is another helper that handles the common
    // case of running a command, waiting for it to finish,
    // and collecting its output. If there were no errors,
    // `dateOut` will hold bytes with the date info.

    // Next we'll look at a slightly more involved case
    // where we pipe data to the external process on its
    // `stdin` and collect the results from its `stdout`.
    amyCmd := exec.Command("./bin/rmini", "-d", "11")

    // Here we explicitly grab input/output pipes, start
    // the process, write some input to it, read the
    // resulting output, and finally wait for the process
    // to exit.
    amyIn, _ := amyCmd.StdinPipe()
    amyOut, _ := amyCmd.StdoutPipe()
    amyCmd.Start()
    amyIn.Write([]byte("?c\n"))
    amyIn.Write([]byte("v0w1f110l1\n"))
    amyIn.Write([]byte("?c\n"))
    time.Sleep(1 * time.Second)
    amyIn.Write([]byte("v0f55\n"))
    amyIn.Write([]byte("?c\n"))
    time.Sleep(1 * time.Second)
    amyIn.Write([]byte("v0f220\n"))
    amyIn.Write([]byte("?c\n"))
    time.Sleep(1 * time.Second)
    amyIn.Close()
    amyBytes, _ := ioutil.ReadAll(amyOut)
    amyCmd.Wait()

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
