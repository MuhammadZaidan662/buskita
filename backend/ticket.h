#pragma once

#include <string>
#include <vector>

// ============================================================
//  KAPASITAS ARRAY TETAP
//  Array dibuat dengan ukuran tetap. Isinya bisa lebih sedikit
//  dari kapasitas (lihat ticketCount dan bookingCount).
// ============================================================
constexpr int MAX_TICKETS  = 50;
constexpr int MAX_BOOKINGS = 100;

// ============================================================
//  STRUCT UTAMA
// ============================================================
struct Ticket {
    std::string kode;
    std::string namaBus;
    std::string asal;
    std::string tujuan;
    std::string tanggal;      // format YYYY-MM-DD
    std::string jam;          // format HH:MM
    int harga;
    int kursiTersedia;
};

struct Booking {
    std::string kodeBooking;  // contoh: BK-0001
    std::string kodeTiket;
    std::string namaPemesan;
    int jumlahKursi;
    int totalHarga;
};

// ============================================================
//  STRUCT LOG (menggantikan cout di dalam algoritma)
//  Algoritma mengisi struct ini, lalu server mengubahnya ke JSON.
// ============================================================

// --- Log Binary Search ---
struct SearchStep {
    std::string fase;         // "binary" atau "telusur"
    int low;                  // pada fase telusur diisi -1
    int high;                 // pada fase telusur diisi -1
    int mid;                  // fase telusur: indeks yang diperiksa
    std::string kode;
    std::string rute;         // "Surabaya -> Jakarta"
    std::string hasil;        // SAMA / LEBIH KECIL / LEBIH BESAR / BEDA
    std::string keputusan;
};

struct SearchLog {
    std::string asal;
    std::string tujuan;
    std::string tanggal;      // kosong = semua tanggal
    std::vector<SearchStep> steps;
    int perbandingan = 0;
    bool ditemukan = false;
    int indeksAwal = -1;      // rentang indeks rute yang cocok
    int indeksAkhir = -1;
    int jumlahRute = 0;       // tiket pada rute (sebelum filter tanggal)
    int jumlahHasil = 0;      // tiket setelah filter tanggal
    bool arrayTerurut = false;
    std::string ringkasan;
};

// --- Log Selection Sort ---
struct SortItem {
    std::string kode;
    int harga;
};

struct CompareStep {
    int j;
    std::string kode;
    int harga;
    int kandidatIdx;
    std::string kandidatKode;
    int kandidatHarga;
    bool ganti;               // true jika kandidat berubah
};

struct SortRound {
    int putaran;
    int posisi;
    int kandidatIdx;
    std::string kandidatKode;
    int kandidatHarga;
    std::vector<CompareStep> komparasi;
    bool tukar;
    std::string kodeI;        // kode di posisi i sebelum pertukaran
    std::string kodeBest;     // kode kandidat sebelum pertukaran
    std::vector<SortItem> keadaan;   // isi array setelah putaran ini
};

struct SortLog {
    bool ascending = true;
    int n = 0;
    std::vector<SortItem> awal;
    std::vector<SortRound> rounds;
    std::vector<SortItem> akhir;
    int perbandingan = 0;
    int pertukaran = 0;
    std::string ringkasan;
};

// --- Hasil pemesanan ---
struct BookingResult {
    bool sukses;
    std::string pesan;
    Booking data;             // hanya valid jika sukses == true
};

// ============================================================
//  DATA GLOBAL (didefinisikan di ticket.cpp)
// ============================================================
extern Ticket tickets[MAX_TICKETS];
extern int ticketCount;

extern Booking bookings[MAX_BOOKINGS];
extern int bookingCount;

// ============================================================
//  FUNGSI (tahap 1: ticket.cpp)
// ============================================================
void tampilkanSemuaTiket();
Ticket* cariTiketByKode(const std::string& kode);

// ============================================================
//  FUNGSI (tahap 2: searching.cpp)
// ============================================================
int bandingkanRute(
    const Ticket* tiket,
    const std::string& asal,
    const std::string& tujuan
);

bool arrayTerurutByRute();

int cariTiketByRute(
    const std::string& asal,
    const std::string& tujuan,
    const std::string& tanggal,       // "" = semua tanggal
    Ticket* hasil[],
    int kapasitas,
    SearchLog* log = nullptr          // nullptr = tanpa log
);

// ============================================================
//  FUNGSI (tahap 3: sorting.cpp)
// ============================================================
void selectionSortHarga(
    Ticket* hasil[],
    int jumlah,
    bool ascending,
    SortLog* log = nullptr            // nullptr = tanpa log
);

// ============================================================
//  FUNGSI (tahap 4: booking.cpp)
// ============================================================
BookingResult buatBooking(
    const std::string& kodeTiket,
    const std::string& namaPemesan,
    int jumlahKursi
);