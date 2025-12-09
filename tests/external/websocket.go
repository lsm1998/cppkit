package main

import (
	"log"
	"net/http"

	"github.com/gorilla/websocket"
)

// 1. 配置 Upgrader
// Upgrader 用于将 HTTP 连接升级为 WebSocket 连接
var upgrader = websocket.Upgrader{
	// 允许跨域请求 (为了方便本地测试，生产环境需严格限制)
	CheckOrigin: func(r *http.Request) bool {
		return true
	},
}

// 2. WebSocket 处理函数
func echoHandler(w http.ResponseWriter, r *http.Request) {
	// 将 HTTP 升级为 WebSocket
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Printf("升级 WebSocket 失败: %v", err)
		return
	}
	defer conn.Close() // 确保连接关闭

	log.Printf("客户端已连接: %s", conn.RemoteAddr())

	// 3. 读写循环
	for {
		// 读取消息
		// messageType: 消息类型 (文本 TextMessage 或 二进制 BinaryMessage)
		// message: 消息内容的字节切片
		messageType, message, err := conn.ReadMessage()
		if err != nil {
			// 如果出现错误（通常是客户端断开连接），跳出循环
			log.Printf("读取错误或连接断开: %v", err)
			break
		}

		log.Printf("收到消息: %s", message)

		// 回写消息 (Echo)
		// 将收到的消息原样发回给客户端
		err = conn.WriteMessage(messageType, message)
		if err != nil {
			log.Printf("写入错误: %v", err)
			break
		}
	}
}

func main() {
	// 注册路由
	http.HandleFunc("/ws", echoHandler)

	// 为了方便测试，我们可以简单地把当前目录作为静态文件服务器
	// 这样你可以直接访问 index.html
	http.Handle("/", http.FileServer(http.Dir(".")))

	log.Println("WebSocket 服务器启动在 :8080")
	log.Println("请访问 http://localhost:8080 进行测试")

	// 启动 HTTP 服务器
	err := http.ListenAndServe(":8080", nil)
	if err != nil {
		log.Fatal("服务器启动失败: ", err)
	}
}