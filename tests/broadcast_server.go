package main

import (
	"bufio"
	"fmt"
	"net"
	"sync"
	"time"
)

type Server struct {
	addr    string
	clients map[net.Conn]bool
	mu      sync.Mutex
}

func NewServer(addr string) *Server {
	return &Server{
		addr:    addr,
		clients: make(map[net.Conn]bool),
	}
}

// 广播给所有客户端
func (s *Server) broadcast(sender net.Conn, msg string) {
	s.mu.Lock()
	defer s.mu.Unlock()
	for c := range s.clients {
		if c == sender {
			continue // 不回显给自己
		}
		_, err := c.Write([]byte(msg))
		if err != nil {
			fmt.Println("[send err]", err)
			c.Close()
			delete(s.clients, c)
		}
	}
}

// 每秒打印当前在线客户端数量
func (s *Server) monitor() {
	for {
		time.Sleep(1 * time.Second)
		s.mu.Lock()
		count := len(s.clients)
		s.mu.Unlock()
		fmt.Printf("[stats] online clients: %d\n", count)
	}
}

// 处理单个客户端
func (s *Server) handleConn(conn net.Conn) {
	s.mu.Lock()
	s.clients[conn] = true
	s.mu.Unlock()

	fmt.Println("[conn]", conn.RemoteAddr(), "connected")

	scanner := bufio.NewScanner(conn)
	for scanner.Scan() {
		msg := scanner.Text()
		fmt.Printf("[msg] %s: %s\n", conn.RemoteAddr(), msg)
		s.broadcast(conn, msg+"\n")
	}

	fmt.Println("[close]", conn.RemoteAddr(), "disconnected")
	s.mu.Lock()
	delete(s.clients, conn)
	s.mu.Unlock()
	conn.Close()
}

// 启动服务器
func (s *Server) Start() error {
	ln, err := net.Listen("tcp", s.addr)
	if err != nil {
		return err
	}
	fmt.Println("Broadcast server running on", s.addr)

	go s.monitor()

	for {
		conn, err := ln.Accept()
		if err != nil {
			fmt.Println("accept err:", err)
			continue
		}
		go s.handleConn(conn)
	}
}

func main() {
	s := NewServer(":6380")
	if err := s.Start(); err != nil {
		fmt.Println("error:", err)
	}
}
