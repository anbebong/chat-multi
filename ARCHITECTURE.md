# Kiến Trúc Hệ Thống Chat

## Sơ Đồ Luồng Xử Lý

```mermaid
---
config:
  layout: dagre
  theme: base
  look: neo
---
flowchart TD
 subgraph subGraph0["Client (src/client/main.c)"]
        C2["Socket Connect"]
        C1["Client Main"]
        S1["Server Main"]
        C4["Send Message"]
        C3["Input Loop"]
        C5["Handle Commands"]
        C7["Parse Message"]
        C6["Receive Thread"]
        C8["Display Message"]
  end
 subgraph subGraph1["Server (src/server/*.c)"]
        S2["Client Handler"]
        S3["Check users.txt"]
        S4["Active Clients"]
        S5["Message Handler"]
        S6["Broadcast Message"]
        S7["Storage"]
        S8["conversations/*.txt"]
  end
    C1 -- Kết nối --> C2
    C1 -- Nhập username/password --> C2
    C2 -- Gửi auth --> S1
    C3 -- Gửi tin nhắn --> C4
    C4 -- MSG|sender|recipient|content --> S1
    C3 -- Xử lý lệnh --> C5
    C5 -- /users --> S1
    C5 -- /history username --> S1
    C5 -- /to username --> S1
    C5 -- /all --> S1
    C6 -- Nhận tin nhắn --> C7
    C7 -- MSG|timestamp|sender|recipient|content --> C8
    C7 -- SYS|content --> C8
    S1 -- Xử lý client --> S2
    S2 -- Xác thực --> S3
    S2 -- Thêm client --> S4
    S2 -- Xử lý tin nhắn --> S5
    S5 -- Parse MSG --> S6
    S6 -- Lưu tin nhắn --> S7
    S7 -- Lưu vào file --> S8
    S6 -- Gửi đến client --> C6
```

## Giải Thích Chi Tiết

### 1. Client Side (src/client/main.c)
- `Client Main`: Khởi tạo kết nối và xác thực
- `Input Loop`: Xử lý input từ người dùng
- `Send Message`: Gửi tin nhắn theo format mới
- `Handle Commands`: Xử lý các lệnh đặc biệt
- `Receive Thread`: Thread riêng để nhận tin nhắn

### 2. Server Side
- `Server Main (src/server/main.c)`: Khởi tạo server
- `Client Handler (src/server/client_handler.c)`: Xử lý từng client
- `Message Handler (src/server/message.c)`: Xử lý tin nhắn
- `Storage (src/server/storage.c)`: Lưu trữ tin nhắn

### 3. Storage Format
- `all.txt`: Tin nhắn công khai
- `user1_user2.txt`: Tin nhắn riêng
- Format: `MSG|timestamp|sender|recipient|content|is_private`

### 4. Luồng Xử Lý Chính
1. Client kết nối và xác thực
2. Server kiểm tra users.txt
3. Client gửi tin nhắn: `MSG|sender|recipient|content`
4. Server thêm timestamp và lưu
5. Server gửi đến client: `MSG|timestamp|sender|recipient|content`
6. Client nhận và hiển thị

### 5. Các File Chính
- `src/client/main.c`: Xử lý client
- `src/server/main.c`: Khởi tạo server
- `src/server/client_handler.c`: Xử lý client
- `src/server/message.c`: Xử lý tin nhắn
- `src/server/storage.c`: Lưu trữ
- `users.txt`: Thông tin người dùng
- `conversations/*.txt`: Lưu trữ tin nhắn