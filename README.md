# Multi-User Chat System / Hệ Thống Chat Đa Người Dùng

[English](#english) | [Tiếng Việt](#tiếng-việt)

<a name="english"></a>
## English

A robust multi-user chat system supporting both private and public messaging, with cross-platform compatibility.

### Features

- Real-time messaging between multiple users
- Private messaging between users
- Public chat room
- User authentication
- Message history storage
- Cross-platform support (Windows and Linux)
- Clean and standardized message format
- Thread-safe implementation
- Conversation management
- User status tracking

### Message Format

The system uses a standardized message format for all communications:

1. Client to Server:
   - Private message: `MSG|sender|recipient|content`
   - System command: `/command [args]`

2. Server to Client:
   - Chat message: `MSG|timestamp|sender|recipient|content`
   - System message: `SYS|content`

### Building the Project

#### Prerequisites

- GCC compiler
- Make
- For cross-compilation: MinGW-w64 (on Linux)

#### Native Build

##### On Linux:
```bash
make clean
make
```

##### On Windows:
```bash
mingw32-make clean
mingw32-make
```

#### Cross-Compilation (from Linux to Windows)

1. Install MinGW-w64:
```bash
sudo apt-get install mingw-w64
```

2. Build for Windows:
   - For 32-bit Windows:
   ```bash
   make win32
   ```
   - For 64-bit Windows:
   ```bash
   make win64
   ```

The executables will be created in the `bin/` directory:
- Linux: `bin/client` and `bin/server`
- Windows: `bin/client.exe` and `bin/server.exe`

### Running the Application

1. Start the server:
```bash
./bin/server
```

2. Start the client(s):
```bash
./bin/client
```

### Client Commands

- `/help` - Show available commands
- `/quit` - Exit the chat
- `/users` - List online users
- `/conv` - Show active conversations
- `/history [username]` - View chat history with a user
- `/to [username]` - Start private chat with a user
- `/all` - Switch to public chat
- `/clear` - Clear the screen

### Project Structure

```
.
├── bin/                    # Compiled executables
├── include/               # Header files
│   ├── client.h          # Client-specific definitions
│   ├── common.h          # Shared definitions
│   └── server.h          # Server-specific definitions
├── src/
│   ├── client/           # Client source files
│   │   ├── main.c       # Client entry point
│   │   ├── message.c    # Message handling
│   │   ├── conversation.c # Conversation management
│   │   └── display.c    # Display functions
│   └── server/           # Server source files
│       ├── main.c       # Server entry point
│       ├── client_handler.c # Client connection handling
│       ├── message.c    # Message broadcasting
│       └── storage.c    # Message storage
├── Makefile              # Build configuration
├── users.txt            # User credentials
└── README.md            # This file
```

### Technical Details

#### Thread Safety
- Uses mutexes for thread synchronization
- Safe handling of shared resources
- Proper cleanup on program exit

#### Network Protocol
- TCP/IP based communication
- Standardized message format
- Robust error handling

#### Storage
- Persistent message history
- User authentication data
- Conversation tracking

### Platform Support

#### Windows
- Native build using MinGW
- Cross-compilation from Linux
- Winsock2 for networking
- Windows-specific thread handling

#### Linux
- Native build using GCC
- POSIX threads
- BSD sockets

### License

This project is licensed under the MIT License - see the LICENSE file for details.

### Contributing

1. Fork the repository
2. Create your feature branch
3. Commit your changes
4. Push to the branch
5. Create a new Pull Request

---

<a name="tiếng-việt"></a>
## Tiếng Việt

Hệ thống chat đa người dùng mạnh mẽ, hỗ trợ cả tin nhắn riêng tư và công khai, với khả năng tương thích đa nền tảng.

### Tính Năng

- Nhắn tin thời gian thực giữa nhiều người dùng
- Nhắn tin riêng tư giữa người dùng
- Phòng chat công khai
- Xác thực người dùng
- Lưu trữ lịch sử tin nhắn
- Hỗ trợ đa nền tảng (Windows và Linux)
- Định dạng tin nhắn chuẩn hóa và rõ ràng
- Triển khai an toàn với luồng
- Quản lý cuộc hội thoại
- Theo dõi trạng thái người dùng

### Định Dạng Tin Nhắn

Hệ thống sử dụng định dạng tin nhắn chuẩn hóa cho mọi giao tiếp:

1. Client đến Server:
   - Tin nhắn riêng tư: `MSG|người_gửi|người_nhận|nội_dung`
   - Lệnh hệ thống: `/lệnh [tham_số]`

2. Server đến Client:
   - Tin nhắn chat: `MSG|thời_gian|người_gửi|người_nhận|nội_dung`
   - Tin nhắn hệ thống: `SYS|nội_dung`

### Biên Dịch Dự Án

#### Yêu Cầu

- Trình biên dịch GCC
- Make
- Cho biên dịch chéo: MinGW-w64 (trên Linux)

#### Biên Dịch Native

##### Trên Linux:
```bash
make clean
make
```

##### Trên Windows:
```bash
mingw32-make clean
mingw32-make
```

#### Biên Dịch Chéo (từ Linux sang Windows)

1. Cài đặt MinGW-w64:
```bash
sudo apt-get install mingw-w64
```

2. Biên dịch cho Windows:
   - Cho Windows 32-bit:
   ```bash
   make win32
   ```
   - Cho Windows 64-bit:
   ```bash
   make win64
   ```

Các file thực thi sẽ được tạo trong thư mục `bin/`:
- Linux: `bin/client` và `bin/server`
- Windows: `bin/client.exe` và `bin/server.exe`

### Chạy Ứng Dụng

1. Khởi động server:
```bash
./bin/server
```

2. Khởi động client:
```bash
./bin/client
```

### Các Lệnh Client

- `/help` - Hiển thị các lệnh có sẵn
- `/quit` - Thoát khỏi chat
- `/users` - Liệt kê người dùng đang online
- `/conv` - Hiển thị các cuộc hội thoại đang hoạt động
- `/history [tên_người_dùng]` - Xem lịch sử chat với một người dùng
- `/to [tên_người_dùng]` - Bắt đầu chat riêng với một người dùng
- `/all` - Chuyển sang chat công khai
- `/clear` - Xóa màn hình

### Cấu Trúc Dự Án

```
.
├── bin/                    # Các file thực thi đã biên dịch
├── include/               # Các file header
│   ├── client.h          # Định nghĩa cho client
│   ├── common.h          # Định nghĩa dùng chung
│   └── server.h          # Định nghĩa cho server
├── src/
│   ├── client/           # Mã nguồn client
│   │   ├── main.c       # Điểm vào của client
│   │   ├── message.c    # Xử lý tin nhắn
│   │   ├── conversation.c # Quản lý hội thoại
│   │   └── display.c    # Các hàm hiển thị
│   └── server/           # Mã nguồn server
│       ├── main.c       # Điểm vào của server
│       ├── client_handler.c # Xử lý kết nối client
│       ├── message.c    # Phát tin nhắn
│       └── storage.c    # Lưu trữ tin nhắn
├── Makefile              # Cấu hình biên dịch
├── users.txt            # Thông tin xác thực người dùng
└── README.md            # File này
```

### Chi Tiết Kỹ Thuật

#### An Toàn Luồng
- Sử dụng mutex để đồng bộ hóa luồng
- Xử lý an toàn tài nguyên dùng chung
- Dọn dẹp đúng cách khi thoát chương trình

#### Giao Thức Mạng
- Giao tiếp dựa trên TCP/IP
- Định dạng tin nhắn chuẩn hóa
- Xử lý lỗi mạnh mẽ

#### Lưu Trữ
- Lưu trữ lịch sử tin nhắn
- Dữ liệu xác thực người dùng
- Theo dõi cuộc hội thoại

### Hỗ Trợ Nền Tảng

#### Windows
- Biên dịch native sử dụng MinGW
- Biên dịch chéo từ Linux
- Sử dụng Winsock2 cho mạng
- Xử lý luồng đặc thù cho Windows

#### Linux
- Biên dịch native sử dụng GCC
- Sử dụng POSIX threads
- Sử dụng BSD sockets

### Giấy Phép

Dự án này được cấp phép theo MIT License - xem file LICENSE để biết thêm chi tiết.

### Đóng Góp

1. Fork repository
2. Tạo nhánh tính năng của bạn
3. Commit các thay đổi
4. Push lên nhánh
5. Tạo Pull Request mới
