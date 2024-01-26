package main

import (
    "fmt"
    "time"
)

func main() {

    ticker := time.NewTicker(250 * time.Millisecond)
    done := make(chan bool)

    go func() {
        l := time.Now()
        for {
            select {
            case <-done:
                return
            case t := <-ticker.C:
                //fmt.Println("Tick at", t)
                //fmt.Println("Last at", l)
                fmt.Println("diff", t.Sub(l).Milliseconds())
                l = t
            }
        }
    }()

    time.Sleep(1000 * time.Millisecond)
    ticker.Stop()
    done <- true
    fmt.Println("Ticker stopped")
}
