package main

import "fmt"
import "C"

func main() {}

//export myHello
func myHello() {
	fmt.Println("Hello from Go!")
}
