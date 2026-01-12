<a name="readme-top"></a>

<!-- PROJECT SHIELDS -->
[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
[![MIT License][license-shield]][license-url]
[![LinkedIn][linkedin-shield]][linkedin-url]



<!-- PROJECT LOGO -->
<br />
<div align="center">
  <a href="https://github.com/AnkurMPansara/SuparnaD">
    <img src="images/logo.png" alt="Logo" width="80" height="80">
  </a>

  <h3 align="center">SuparnaD</h3>

  <p align="center">
    A high-performance pub/sub message broker written in C, built for low-latency delivery with socket-based streaming and chunked, memory-efficient storage.
    <br />
    <a href="https://github.com/AnkurMPansara/SuparnaD"><strong>Explore the docs »</strong></a>
    <br />
    <br />
    <a href="https://github.com/AnkurMPansara/SuparnaD">View Demo</a>
    ·
    <a href="https://github.com/AnkurMPansara/SuparnaD/issues">Report Bug</a>
    ·
    <a href="https://github.com/AnkurMPansara/SuparnaD/issues">Request Feature</a>
  </p>
</div>



<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
      <ul>
        <li><a href="#built-with">Built With</a></li>
        <li><a href="#features">Features</a></li>
      </ul>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
      </ul>
    </li>
    <li><a href="#usage">Usage</a></li>
    <li><a href="#architecture">Architecture</a></li>
    <li><a href="#api-reference">API Reference</a></li>
    <li><a href="#roadmap">Roadmap</a></li>
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
    <li><a href="#acknowledgments">Acknowledgments</a></li>
  </ol>
</details>



<!-- ABOUT THE PROJECT -->
## About The Project

SuparnaD is a high-performance publish/subscribe message broker designed for low-latency message delivery. Built entirely in C, it provides:

* **Socket-based streaming** for real-time consumer connections
* **Chunked storage** with memory-efficient delta writes
* **Consumer groups** with independent read pointers and acknowledgment tracking
* **Zstd compression** support for efficient data storage
* **HTTP REST API** for easy integration
* **Thread-safe operations** with mutex-protected data structures

Perfect for applications requiring high-throughput message processing, real-time data streaming, and efficient storage management.

<p align="right">(<a href="#readme-top">back to top</a>)</p>



### Built With

* [libmicrohttpd](https://www.gnu.org/software/libmicrohttpd/) - HTTP server library
* [Zstd](https://github.com/facebook/zstd) - Fast compression algorithm
* C Standard Library - Core functionality

<p align="right">(<a href="#readme-top">back to top</a>)</p>

### Features

* ✅ **Topic Management** - Create and manage topics with persistent storage
* ✅ **Publish/Subscribe** - Publish events via HTTP API, consume via socket streaming
* ✅ **Consumer Groups** - Multiple consumer groups per topic with independent read pointers
* ✅ **Packet Acknowledgment** - Reliable message delivery with ACK support
* ✅ **Compression** - Optional Zstd compression for efficient storage
* ✅ **Batch Operations** - Batch publish and acknowledge multiple packets
* ✅ **Thread-Safe** - Mutex-protected operations for concurrent access
* ✅ **Memory Efficient** - Chunked storage with delta writes

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- GETTING STARTED -->
## Getting Started

### Prerequisites

* GCC compiler (C11 or later)
* CMake (optional, for build system)
* libmicrohttpd development libraries
* Zstd development libraries

### Installation

1. Clone the repo
   ```sh
   git clone https://github.com/AnkurMPansara/SuparnaD.git
   cd SuparnaD
   ```

2. Install dependencies
   
   **Ubuntu/Debian:**
   ```sh
   sudo apt-get install libmicrohttpd-dev libzstd-dev build-essential
   ```
   
   **macOS:**
   ```sh
   brew install libmicrohttpd zstd
   ```

3. Build the project
   ```sh
   gcc -o suparnad src/main.c src/api/*.c src/messaging/*.c src/socket/*.c src/writer/*.c src/utils/*.c -lmicrohttpd -lzstd -lpthread
   ```

4. Run the server
   ```sh
   ./suparnad
   ```

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- USAGE EXAMPLES -->
## Usage

### HTTP API

#### Create a Topic
```bash
curl -X POST http://localhost:8080/create_topic \
  -H "Content-Type: application/json" \
  -d '{"topic": "events", "base_path": "./topics"}'
```

#### Publish an Event
```bash
curl -X POST http://localhost:8080/publish \
  -H "Content-Type: application/json" \
  -d '{"topic": "events", "data": "{\"key\":\"value\"}"}'
```

### Socket Consumer

Connect to the socket server and use commands:

```
SET_TOPIC events
SET_GROUP consumer_group_1
CONSUME
ACK
```

### Code Example

```c
#include "messaging/headers/create_topic.h"
#include "messaging/headers/publish_event.h"
#include "messaging/headers/manage_groups.h"
#include "messaging/headers/consume_packet.h"

// Create a topic
Topic* topic = create_topic("my_topic", "./topics");

// Publish an event
publish_event_string(topic, "Hello, World!");

// Create a consumer group
Group* group = create_group("group1", topic);

// Consume packets
Packet* packet = consume_packet(group, topic);
if (packet) {
    // Process packet->data
    ack_packet(group, packet);
    packet_free(packet);
}
```

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- ARCHITECTURE -->
## Architecture

SuparnaD is organized into several modules:

* **`src/api/`** - HTTP REST API handlers using libmicrohttpd
* **`src/messaging/`** - Core messaging functionality (topics, groups, publish, consume, ack)
* **`src/socket/`** - Socket-based streaming server for consumers
* **`src/writer/`** - Chunked file I/O with delta writes
* **`src/utils/`** - Utility functions and logging

### Data Flow

1. **Publisher** → HTTP POST `/publish` → Topic storage
2. **Consumer** → Socket connection → Consume packets → ACK → Update read pointer
3. **Storage** → Chunked files → Delta writes → Memory-efficient persistence

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- API REFERENCE -->
## API Reference

### HTTP Endpoints

#### `POST /create_topic`
Create a new topic.

**Request:**
```json
{
  "topic": "topic_name",
  "base_path": "./topics"  // optional
}
```

**Response:**
```json
{
  "status": "success",
  "message": "Topic created successfully",
  "topic": "topic_name",
  "path": "./topics/topic_name.topic"
}
```

#### `POST /publish`
Publish an event to a topic.

**Request:**
```json
{
  "topic": "topic_name",
  "data": "event data"
}
```

**Response:**
```json
{
  "status": "success",
  "message": "Event published successfully"
}
```

### Socket Commands

- `SET_TOPIC <topic_name>` - Set the topic for the session
- `SET_GROUP <group_id>` - Set the consumer group
- `CONSUME` - Consume the next packet
- `ACK` - Acknowledge the last consumed packet
- `QUIT` - Close the connection

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- ROADMAP -->
## Roadmap

- [x] Core messaging functionality
- [x] HTTP REST API
- [x] Socket-based streaming
- [x] Consumer groups
- [x] Packet acknowledgment
- [x] Zstd compression support
- [ ] WebSocket support
- [ ] Message persistence with WAL
- [ ] Clustering and replication
- [ ] Admin dashboard
- [ ] Performance benchmarks
- [ ] Docker containerization

See the [open issues](https://github.com/AnkurMPansara/SuparnaD/issues) for a full list of proposed features (and known issues).

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- CONTRIBUTING -->
## Contributing

Contributions are what make the open source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

If you have a suggestion that would make this better, please fork the repo and create a pull request. You can also simply open an issue with the tag "enhancement".
Don't forget to give the project a star! Thanks again!

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- LICENSE -->
## License

Distributed under the MIT License. See `LICENSE.txt` for more information.

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- CONTACT -->
## Contact

Ankur Pansara - [@AnkurMPansara](https://github.com/AnkurMPansara) - ankur.at.surat@gmail.com

Project Link: [https://github.com/AnkurMPansara/SuparnaD](https://github.com/AnkurMPansara/SuparnaD)

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- ACKNOWLEDGMENTS -->
## Acknowledgments

Use this space to list resources you find helpful and would like to give credit to. I've included a few of my favorites to kick things off!

* [libmicrohttpd](https://www.gnu.org/software/libmicrohttpd/) - Lightweight HTTP server library
* [Zstd](https://github.com/facebook/zstd) - Fast compression library by Facebook
* [Choose an Open Source License](https://choosealicense.com)
* [GitHub Emoji Cheat Sheet](https://www.webpagefx.com/tools/emoji-cheat-sheet)
* [Img Shields](https://shields.io)
* [GitHub Pages](https://pages.github.com)

<p align="right">(<a href="#readme-top">back to top</a>)</p>



<!-- MARKDOWN LINKS & IMAGES -->
<!-- https://www.markdownguide.org/basic-syntax/#reference-style-links -->
[contributors-shield]: https://img.shields.io/github/contributors/AnkurMPansara/SuparnaD.svg?style=for-the-badge
[contributors-url]: https://github.com/AnkurMPansara/SuparnaD/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/AnkurMPansara/SuparnaD.svg?style=for-the-badge
[forks-url]: https://github.com/AnkurMPansara/SuparnaD/network/members
[stars-shield]: https://img.shields.io/github/stars/AnkurMPansara/SuparnaD.svg?style=for-the-badge
[stars-url]: https://github.com/AnkurMPansara/SuparnaD/stargazers
[issues-shield]: https://img.shields.io/github/issues/AnkurMPansara/SuparnaD.svg?style=for-the-badge
[issues-url]: https://github.com/AnkurMPansara/SuparnaD/issues
[license-shield]: https://img.shields.io/github/license/AnkurMPansara/SuparnaD.svg?style=for-the-badge
[license-url]: https://github.com/AnkurMPansara/SuparnaD/blob/master/LICENSE.txt
[linkedin-shield]: https://img.shields.io/badge/-LinkedIn-black.svg?style=for-the-badge&logo=linkedin&colorB=555
[linkedin-url]: https://www.linkedin.com/in/ankur-pansara
[product-screenshot]: images/screenshot.png
