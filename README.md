# BusKita

Aplikasi web sederhana untuk pencarian dan pemesanan tiket bus antarkota (tema: Tiketing, mirip Traveloka).
Tugas UTS **Struktur Data** — Pendidikan Teknologi Informasi, Universitas Negeri Surabaya.

- Nama : Muhammad Zaidan
- NIM  : 25050974026
- Video demo : ...

## Fitur

- Beranda dengan statistik, carousel, dan pencarian cepat
- Cari tiket berdasarkan kota asal, kota tujuan, dan tanggal (opsional)
- Urutkan hasil dari harga termurah atau termahal
- Pencarian kata kunci (kota, kode tiket, atau nama operator) dari kolom header
- Pemesanan tiket dan riwayat booking
- **Log Sorting** dan **Log Searching**: langkah algoritma ditampilkan apa adanya dari eksekusi di C++

## Konsep Struktur Data yang Dipakai

Semua struktur data dan algoritma berjalan di **backend C++**. Frontend hanya menampilkan hasil JSON dari server.

| Konsep | Penerapan | File |
|---|---|---|
| Array | `Ticket tickets[MAX_TICKETS]` (kapasitas 50, terisi 40) dan `Booking bookings[MAX_BOOKINGS]` | `ticket.cpp` |
| Struct | `Ticket`, `Booking`, serta struct log `SearchLog` dan `SortLog` | `ticket.h` |
| Pointer | Hasil pencarian disimpan sebagai `Ticket* hasil[]` (alamat `&tickets[i]`); kursi berkurang lewat pointer saat booking | `searching.cpp`, `sorting.cpp`, `booking.cpp` |
| Searching | **Binary Search** manual berdasarkan rute (asal + tujuan), lalu menelusuri tetangga kiri dan kanan | `searching.cpp` |
| Sorting | **Selection Sort** manual berdasarkan harga (naik atau turun) | `sorting.cpp` |

Catatan desain:

- Array utama `tickets[]` selalu terurut menurut rute, karena itu syarat Binary Search.
- Selection Sort hanya menukar **pointer** pada salinan hasil pencarian, sehingga array utama tetap terurut menurut rute.
- Tidak memakai `std::sort` maupun `std::binary_search`. `std::vector` hanya dipakai untuk menyimpan catatan log, bukan untuk data tiket.
- Log ditulis oleh algoritma itu sendiri saat berjalan, bukan dibuat ulang di frontend.

## Struktur Folder

```
BusKita/
├── backend/
│   ├── ticket.h        # struct, konstanta, deklarasi fungsi
│   ├── ticket.cpp      # data array tiket dan booking
│   ├── searching.cpp   # Binary Search + log
│   ├── sorting.cpp     # Selection Sort + log
│   ├── booking.cpp     # logika pemesanan
│   ├── server.cpp      # server HTTP dan endpoint API (main)
│   ├── httplib.h       # cpp-httplib (library pihak ketiga)
│   └── json.hpp        # nlohmann/json (library pihak ketiga)
├── frontend/
│   ├── index.html
│   ├── style.css
│   └── app.js
└── README.md
```

## Cara Menjalankan (Windows, MinGW-w64 / g++)

Prasyarat: `g++` dengan dukungan C++17 (MinGW-w64).

1. Buka terminal di folder `BusKita`.
2. Kompilasi:

```powershell
   g++ -std=c++17 -Wall -Wextra -D_WIN32_WINNT=0x0A00 backend/server.cpp backend/ticket.cpp backend/searching.cpp backend/sorting.cpp backend/booking.cpp -o backend/buskita.exe -lws2_32
```

3. Jalankan server:

```powershell
   .\backend\buskita.exe
```

   Untuk port lain: `.\backend\buskita.exe 8081`

4. Buka **http://127.0.0.1:8080** di browser. Jangan membuka `index.html` dengan klik dua kali, karena halaman memanggil API dari server.
5. Hentikan server dengan `Ctrl+C`.

Data bersifat sementara: pemesanan dan log kembali kosong setiap server dijalankan ulang.

## Endpoint API

Semua jawaban berbentuk `{ "success": true/false, "message": "...", "data": ... }`.

| Method | Alamat | Fungsi |
|---|---|---|
| GET | `/api/stats` | Statistik beranda |
| GET | `/api/cities` | Daftar kota |
| GET | `/api/routes` | Daftar rute |
| GET | `/api/tickets` | Semua tiket (urutan array utama) |
| GET | `/api/tickets/search?asal=&tujuan=&tanggal=&urut=asc\|desc` | Binary Search + Selection Sort, beserta kedua log |
| GET | `/api/tickets/keyword?q=` | Pencarian kata kunci |
| GET | `/api/tickets/{kode}` | Detail satu tiket |
| POST | `/api/bookings` | Buat pemesanan (`{"kode","nama","jumlah"}`) |
| GET | `/api/bookings` | Riwayat pemesanan |
| GET | `/api/logs` | Log sorting dan searching terakhir |
| DELETE | `/api/logs` | Hapus log |

## Teknologi

- Backend: C++17, [cpp-httplib](https://github.com/yhirose/cpp-httplib) dan [nlohmann/json](https://github.com/nlohmann/json) (keduanya berlisensi MIT)
- Frontend: HTML, CSS, JavaScript murni (tanpa framework)