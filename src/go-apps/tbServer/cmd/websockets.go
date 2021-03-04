package trueblocks

import (
	"fmt"
	"log"
	"net"
	"net/http"

	"github.com/gorilla/websocket"
)

type MessageType string

const (
	CommandErrorMessage  MessageType = "command_error"
	CommandOutputMessage MessageType = "output"
	ProgressMessage      MessageType = "progress"
)

var upgrader = websocket.Upgrader{}

type Message struct {
	Action   MessageType      `json:"action"`
	Id       string           `json:"id"`
	Content  string           `json:"content"`
	Progress *CommandProgress `json:"progress"`
}

type Connection struct {
	connection *websocket.Conn
	send       chan *Message
	pool       *ConnectionPool
}

func (c *Connection) write() {

	defer func() {
		c.connection.Close()
	}()

	for {
		select {
		case message, ok := <-c.send:
			if !ok {
				c.Log("Connection closed")
				c.connection.WriteMessage(websocket.CloseMessage, []byte{})
				return
			}

			err := c.connection.WriteJSON(message)

			if err != nil {
				c.Log("Error while sending message, dropping connection: %s", err.Error())
				c.pool.unregister <- c
			}
		}
	}
}

func (c *Connection) RemoteAddr() net.Addr {
	return c.connection.RemoteAddr()
}

func (c *Connection) Log(s string, args ...interface{}) {
	log.Printf("%s %s\n", c.RemoteAddr(), fmt.Sprintf(s, args...))
}

type ConnectionPool struct {
	connections map[*Connection]bool
	broadcast   chan *Message
	register    chan *Connection
	unregister  chan *Connection
}

func closeAndDelete(pool *ConnectionPool, connection *Connection) {
	delete(pool.connections, connection)
	close(connection.send)
}

func newConnectionPool() *ConnectionPool {
	return &ConnectionPool{
		connections: make(map[*Connection]bool),
		broadcast:   make(chan *Message),
		register:    make(chan *Connection),
		unregister:  make(chan *Connection),
	}
}

func (pool *ConnectionPool) run() {
	for {
		select {
		case connection := <-pool.register:
			connection.Log("Connected (Websockets)")
			pool.connections[connection] = true
		case connection := <-pool.unregister:
			if _, ok := pool.connections[connection]; ok {
				connection.Log("Unregistering connection")
				closeAndDelete(pool, connection)
			}
		case message := <-pool.broadcast:
			for connection := range pool.connections {
				connection.send <- message
			}
		}
	}
}

func HandleWebsockets(pool *ConnectionPool, w http.ResponseWriter, r *http.Request) {
	// TODO: the line below allows any connection through WebSockets. Once the server
	// is ready, we should implement some rules here
	upgrader.CheckOrigin = func(r *http.Request) bool { return true }

	c, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Print("upgrade:", err)
		return
	}

	connection := &Connection{connection: c, send: make(chan *Message), pool: pool}
	pool.register <- connection

	go connection.write()
}
