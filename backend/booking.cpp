#include "ticket.h"

// ============================================================
//  Fungsi bantu (hanya dipakai di file ini karena "static")
// ============================================================

// Membuang spasi/tab/enter di awal dan akhir teks.
static std::string rapikanNama(const std::string& teks) {
    const std::string pemisah = " \t\r\n";

    std::string::size_type awal = teks.find_first_not_of(pemisah);
    if (awal == std::string::npos) {
        return "";                       // isinya spasi semua
    }

    std::string::size_type akhir = teks.find_last_not_of(pemisah);
    return teks.substr(awal, akhir - awal + 1);
}

// Membuat kode booking: 1 -> "BK-0001", 25 -> "BK-0025".
static std::string formatKodeBooking(int nomor) {
    std::string angka = std::to_string(nomor);

    while (angka.size() < 4) {
        angka = "0" + angka;
    }

    return "BK-" + angka;
}

// Membuat hasil gagal dengan pesan tertentu.
static BookingResult gagal(const std::string& pesan) {
    return BookingResult{false, pesan, Booking{}};
}

// ============================================================
//  PEMESANAN TIKET
//
//  Urutan pengecekan:
//   1. tiket harus ada
//   2. nama tidak boleh kosong (maks. 40 karakter)
//   3. jumlah kursi minimal 1
//   4. kursi harus cukup
//   5. array booking tidak boleh penuh
//
//  Jika semua lolos:
//   - kursi tiket dikurangi (lewat POINTER ke tiket asli)
//   - booking baru disimpan di array bookings[]
//   - kode booking dibuat unik: BK-0001, BK-0002, ...
//
//  Jika ada yang gagal, tidak ada data yang berubah.
// ============================================================
BookingResult buatBooking(
    const std::string& kodeTiket,
    const std::string& namaPemesan,
    int jumlahKursi
) {
    Ticket* tiket = cariTiketByKode(kodeTiket);

    if (tiket == nullptr) {
        return gagal("Tiket " + kodeTiket + " tidak ditemukan.");
    }

    std::string nama = rapikanNama(namaPemesan);

    if (nama.empty()) {
        return gagal("Nama pemesan wajib diisi.");
    }

    if (nama.size() > 40) {
        return gagal("Nama pemesan maksimal 40 karakter.");
    }

    if (jumlahKursi < 1) {
        return gagal("Jumlah kursi minimal 1.");
    }

    if (jumlahKursi > tiket->kursiTersedia) {
        return gagal(
            "Kursi tidak cukup. Tersisa "
            + std::to_string(tiket->kursiTersedia)
            + " kursi untuk " + tiket->kode + "."
        );
    }

    if (bookingCount >= MAX_BOOKINGS) {
        return gagal("Kapasitas array pemesanan penuh.");
    }

    // ---------- Semua syarat terpenuhi: simpan pemesanan ----------
    tiket->kursiTersedia -= jumlahKursi;       // mengubah tiket ASLI di array utama

    Booking* baru = &bookings[bookingCount];   // pointer ke slot kosong berikutnya
    baru->kodeBooking = formatKodeBooking(bookingCount + 1);
    baru->kodeTiket = tiket->kode;
    baru->namaPemesan = nama;
    baru->jumlahKursi = jumlahKursi;
    baru->totalHarga = jumlahKursi * tiket->harga;

    bookingCount++;

    return BookingResult{true, "Pemesanan berhasil.", *baru};
}