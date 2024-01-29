package main

import (
    "fmt"
    "time"
)

func main() {

    ticker := time.NewTicker(250 * time.Millisecond)
    done := make(chan bool)

    g0 := 100

    go func() {
        l := time.Now()
        for {
            select {
            case <- done:
                return
            case t := <- ticker.C:
                //fmt.Println("Tick at", t)
                //fmt.Println("Last at", l)
                fmt.Println("[", g0, "]", "diff", t.Sub(l).Milliseconds())
                l = t
            }
        }
    }()

    time.Sleep(500 * time.Millisecond)
    g0 = 200
    time.Sleep(500 * time.Millisecond)
    g0 = 355
    time.Sleep(500 * time.Millisecond)
    g0 = 113
    time.Sleep(500 * time.Millisecond)
    ticker.Stop()
    done <- true
    fmt.Println("Ticker stopped")
}
