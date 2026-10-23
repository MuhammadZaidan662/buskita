#include "ticket.h"
#include <iostream>

Ticket tickets[MAX_TICKETS] = {
    {
        "BDG-001", "Lintas Nusa",
        "Bandung", "Jakarta",
        "2026-10-25", "07:00", 95000, 20
    },
    {
        "BDG-002", "Rapi Trans",
        "Bandung", "Jakarta",
        "2026-10-25", "13:00", 110000, 12
    },
    {
        "BDG-004", "Cahaya Raya",
        "Bandung", "Jakarta",
        "2026-10-26", "09:30", 85000, 16
    },
    {
        "BDG-005", "Nusantara Express",
        "Bandung", "Jakarta",
        "2026-10-27", "16:00", 125000, 11
    },
    {
        "BDG-003", "Cahaya Raya",
        "Bandung", "Yogyakarta",
        "2026-10-26", "19:00", 185000, 18
    },
    {
        "JKT-001", "Lintas Nusa",
        "Jakarta", "Bandung",
        "2026-10-25", "08:00", 95000, 15
    },
    {
        "JKT-002", "Nusantara Express",
        "Jakarta", "Bandung",
        "2026-10-25", "15:30", 120000, 9
    },
    {
        "JKT-011", "Rapi Trans",
        "Jakarta", "Bandung",
        "2026-10-26", "10:00", 90000, 19
    },
    {
        "JKT-012", "Bintang Timur",
        "Jakarta", "Bandung",
        "2026-10-27", "18:30", 135000, 7
    },
    {
        "JKT-003", "Rapi Trans",
        "Jakarta", "Semarang",
        "2026-10-26", "20:00", 210000, 22
    },
    {
        "JKT-004", "Cahaya Raya",
        "Jakarta", "Surabaya",
        "2026-10-26", "17:00", 330000, 16
    },
    {
        "JKT-005", "Bintang Timur",
        "Jakarta", "Surabaya",
        "2026-10-26", "19:30", 285000, 10
    },
    {
        "JKT-007", "Nusantara Express",
        "Jakarta", "Surabaya",
        "2026-10-25", "20:00", 300000, 14
    },
    {
        "JKT-008", "Lintas Nusa",
        "Jakarta", "Surabaya",
        "2026-10-26", "06:30", 350000, 9
    },
    {
        "JKT-009", "Rapi Trans",
        "Jakarta", "Surabaya",
        "2026-10-27", "14:00", 270000, 22
    },
    {
        "JKT-010", "Bintang Timur",
        "Jakarta", "Surabaya",
        "2026-10-27", "21:30", 295000, 6
    },
    {
        "JKT-006", "Nusantara Express",
        "Jakarta", "Yogyakarta",
        "2026-10-27", "18:00", 250000, 14
    },
    {
        "MLG-001", "Bintang Timur",
        "Malang", "Surabaya",
        "2026-10-25", "06:00", 55000, 25
    },
    {
        "MLG-002", "Lintas Nusa",
        "Malang", "Surabaya",
        "2026-10-25", "10:00", 55000, 8
    },
    {
        "MLG-004", "Cahaya Raya",
        "Malang", "Surabaya",
        "2026-10-26", "08:00", 60000, 13
    },
    {
        "MLG-005", "Rapi Trans",
        "Malang", "Surabaya",
        "2026-10-27", "15:30", 45000, 17
    },
    {
        "MLG-003", "Cahaya Raya",
        "Malang", "Yogyakarta",
        "2026-10-27", "20:00", 175000, 11
    },
    {
        "SMG-001", "Rapi Trans",
        "Semarang", "Jakarta",
        "2026-10-25", "21:00", 200000, 19
    },
    {
        "SMG-002", "Bintang Timur",
        "Semarang", "Surabaya",
        "2026-10-26", "09:00", 120000, 13
    },
    {
        "SBY-001", "Cahaya Raya",
        "Surabaya", "Jakarta",
        "2026-10-25", "16:00", 320000, 17
    },
    {
        "SBY-002", "Nusantara Express",
        "Surabaya", "Jakarta",
        "2026-10-25", "18:30", 295000, 7
    },
    {
        "SBY-006", "Bintang Timur",
        "Surabaya", "Jakarta",
        "2026-10-25", "21:00", 275000, 12
    },
    {
        "SBY-007", "Lintas Nusa",
        "Surabaya", "Jakarta",
        "2026-10-26", "07:30", 340000, 18
    },
    {
        "SBY-008", "Rapi Trans",
        "Surabaya", "Jakarta",
        "2026-10-26", "13:00", 260000, 5
    },
    {
        "SBY-009", "Cahaya Raya",
        "Surabaya", "Jakarta",
        "2026-10-27", "09:00", 310000, 20
    },
    {
        "SBY-003", "Bintang Timur",
        "Surabaya", "Malang",
        "2026-10-25", "08:00", 55000, 21
    },
    {
        "SBY-004", "Lintas Nusa",
        "Surabaya", "Malang",
        "2026-10-25", "14:00", 60000, 6
    },
    {
        "SBY-010", "Nusantara Express",
        "Surabaya", "Malang",
        "2026-10-25", "08:30", 65000, 21
    },
    {
        "SBY-011", "Lintas Nusa",
        "Surabaya", "Malang",
        "2026-10-26", "12:00", 50000, 10
    },
    {
        "SBY-005", "Rapi Trans",
        "Surabaya", "Yogyakarta",
        "2026-10-26", "07:00", 150000, 20
    },
    {
        "SBY-012", "Bintang Timur",
        "Surabaya", "Yogyakarta",
        "2026-10-25", "22:00", 165000, 15
    },
    {
        "SBY-013", "Nusantara Express",
        "Surabaya", "Yogyakarta",
        "2026-10-27", "06:00", 140000, 8
    },
    {
        "YGY-001", "Cahaya Raya",
        "Yogyakarta", "Surabaya",
        "2026-10-28", "09:00", 150000, 15
    },
    {
        "YGY-002", "Lintas Nusa",
        "Yogyakarta", "Surabaya",
        "2026-10-26", "11:00", 155000, 12
    },
    {
        "YGY-003", "Rapi Trans",
        "Yogyakarta", "Surabaya",
        "2026-10-27", "20:30", 135000, 19
    }
};

// Jumlah tiket yang benar-benar terisi di array (kapasitas array = MAX_TICKETS).
int ticketCount = 40;

// Array pemesanan: awalnya kosong.
Booking bookings[MAX_BOOKINGS];
int bookingCount = 0;

void tampilkanSemuaTiket() {
    std::cout << "\n=== DAFTAR TIKET BUS ===\n";

    for (int i = 0; i < ticketCount; i++) {
        Ticket* current = &tickets[i];

        std::cout << "\nIndeks         : " << i << '\n';
        std::cout << "Kode tiket     : " << current->kode << '\n';
        std::cout << "Nama bus       : " << current->namaBus << '\n';
        std::cout << "Rute           : " << current->asal
                  << " -> " << current->tujuan << '\n';
        std::cout << "Tanggal        : " << current->tanggal << '\n';
        std::cout << "Jam            : " << current->jam << '\n';
        std::cout << "Harga          : Rp" << current->harga << '\n';
        std::cout << "Kursi tersedia : "
                  << current->kursiTersedia << '\n';
    }
}

// Mencari tiket berdasarkan kode (pencarian linear biasa).
// Mengembalikan POINTER ke tiket di array utama, atau nullptr jika tidak ada.
Ticket* cariTiketByKode(const std::string& kode) {
    for (int i = 0; i < ticketCount; i++) {
        if (tickets[i].kode == kode) {
            return &tickets[i];
        }
    }
    return nullptr;
}