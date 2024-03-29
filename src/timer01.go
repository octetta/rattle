package main
import (
  "fmt"
  "time"
)

func main() {
  timer1 := time.NewTimer(2 * time.Second)
  <-timer1.C
  fmt.Println("timer1!")
  timer2 := time.NewTimer(time.Second)
  go func() {
    <-timer2.C
    fmt.Println("timer2!")
  }()
  stop2 := timer2.Stop()
  if stop2 {
    fmt.Println("timer2 stopped")
  }
  time.Sleep(2 * time.Second)
}
