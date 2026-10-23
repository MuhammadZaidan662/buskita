#include "ticket.h"

// ============================================================
//  Membandingkan rute sebuah tiket dengan rute yang dicari.
//  Kunci urutan: kota asal dulu, baru kota tujuan.
//  Mengembalikan:
//    -1 -> rute tiket LEBIH KECIL dari target
//     0 -> rute tiket SAMA dengan target
//     1 -> rute tiket LEBIH BESAR dari target
//  Parameter tiket berupa POINTER ke elemen array (akses dengan ->).
// ============================================================
int bandingkanRute(
    const Ticket* tiket,
    const std::string& asal,
    const std::string& tujuan
) {
    if (tiket->asal < asal) {
        return -1;
    }

    if (tiket->asal > asal) {
        return 1;
    }

    if (tiket->tujuan < tujuan) {
        return -1;
    }

    if (tiket->tujuan > tujuan) {
        return 1;
    }

    return 0;
}

// ============================================================
//  Memeriksa apakah array utama terurut menurut kunci rute.
//  Ini adalah SYARAT Binary Search.
// ============================================================
bool arrayTerurutByRute() {
    for (int i = 1; i < ticketCount; i++) {
        if (bandingkanRute(&tickets[i], tickets[i - 1].asal, tickets[i - 1].tujuan) < 0) {
            return false;
        }
    }

    return true;
}

// ============================================================
//  BINARY SEARCH berdasarkan rute (asal + tujuan).
//
//  Langkah:
//   1. Binary Search pada array utama untuk menemukan satu tiket
//      dengan rute yang cocok.
//   2. Dari posisi itu, telusuri ke kiri dan ke kanan untuk
//      mengumpulkan semua tiket dengan rute yang sama.
//   3. Terapkan filter tanggal (jika tanggal tidak kosong).
//   4. Simpan POINTER ke tiket yang cocok di array hasil[].
//
//  Mengembalikan jumlah tiket di hasil[].
//  Jika log != nullptr, seluruh langkah dicatat ke log.
// ============================================================
int cariTiketByRute(
    const std::string& asal,
    const std::string& tujuan,
    const std::string& tanggal,
    Ticket* hasil[],
    int kapasitas,
    SearchLog* log
) {
    if (log != nullptr) {
        log->asal = asal;
        log->tujuan = tujuan;
        log->tanggal = tanggal;
        log->arrayTerurut = arrayTerurutByRute();
    }

    if (kapasitas <= 0 || ticketCount <= 0) {
        if (log != nullptr) {
            log->ringkasan = "Tidak ada data untuk dicari.";
        }
        return 0;
    }

    int low = 0;
    int high = ticketCount - 1;
    int ditemukan = -1;
    int perbandingan = 0;

    // ---------- FASE 1: Binary Search ----------
    while (low <= high) {
        int mid = low + (high - low) / 2;
        Ticket* current = &tickets[mid];

        int c = bandingkanRute(current, asal, tujuan);
        perbandingan++;

        std::string ruteMid = current->asal + " -> " + current->tujuan;
        std::string keterangan;
        std::string hasilBanding;

        if (c == 0) {
            hasilBanding = "SAMA";
            keterangan = "Cocok di indeks " + std::to_string(mid)
                       + ". Telusuri tetangga kiri dan kanan.";
            ditemukan = mid;
        } else if (c < 0) {
            hasilBanding = "LEBIH KECIL";
            keterangan = "Rute[mid] < target, buang separuh kiri: low = "
                       + std::to_string(mid + 1);
        } else {
            hasilBanding = "LEBIH BESAR";
            keterangan = "Rute[mid] > target, buang separuh kanan: high = "
                       + std::to_string(mid - 1);
        }

        if (log != nullptr) {
            log->steps.push_back(
                SearchStep{"binary", low, high, mid, current->kode,
                           ruteMid, hasilBanding, keterangan}
            );
        }

        if (c == 0) {
            break;
        } else if (c < 0) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    if (ditemukan == -1) {
        if (log != nullptr) {
            log->perbandingan = perbandingan;
            log->ditemukan = false;
            log->ringkasan = "Rute " + asal + " -> " + tujuan
                           + " tidak ditemukan setelah "
                           + std::to_string(perbandingan) + " perbandingan.";
        }
        return 0;
    }

    // ---------- FASE 2: telusuri ke kiri ----------
    int kiri = ditemukan;
    while (kiri > 0) {
        Ticket* tetangga = &tickets[kiri - 1];
        bool sama = (bandingkanRute(tetangga, asal, tujuan) == 0);
        perbandingan++;

        if (log != nullptr) {
            log->steps.push_back(
                SearchStep{"telusur", -1, -1, kiri - 1, tetangga->kode,
                           tetangga->asal + " -> " + tetangga->tujuan,
                           sama ? "SAMA" : "BEDA",
                           sama ? "Masih rute yang sama, geser ke kiri"
                                : "Rute berbeda, berhenti di sisi kiri"}
            );
        }

        if (!sama) {
            break;
        }
        kiri--;
    }

    // ---------- FASE 2: telusuri ke kanan ----------
    int kanan = ditemukan;
    while (kanan < ticketCount - 1) {
        Ticket* tetangga = &tickets[kanan + 1];
        bool sama = (bandingkanRute(tetangga, asal, tujuan) == 0);
        perbandingan++;

        if (log != nullptr) {
            log->steps.push_back(
                SearchStep{"telusur", -1, -1, kanan + 1, tetangga->kode,
                           tetangga->asal + " -> " + tetangga->tujuan,
                           sama ? "SAMA" : "BEDA",
                           sama ? "Masih rute yang sama, geser ke kanan"
                                : "Rute berbeda, berhenti di sisi kanan"}
            );
        }

        if (!sama) {
            break;
        }
        kanan++;
    }

    // ---------- FASE 3: filter tanggal + simpan pointer ----------
    int jumlahRute = kanan - kiri + 1;
    int jumlah = 0;

    for (int i = kiri; i <= kanan; i++) {
        if (!tanggal.empty() && tickets[i].tanggal != tanggal) {
            continue;
        }

        if (jumlah < kapasitas) {
            hasil[jumlah] = &tickets[i];   // simpan ALAMAT tiket, bukan salinan
            jumlah++;
        }
    }

    if (log != nullptr) {
        log->perbandingan = perbandingan;
        log->ditemukan = true;
        log->indeksAwal = kiri;
        log->indeksAkhir = kanan;
        log->jumlahRute = jumlahRute;
        log->jumlahHasil = jumlah;

        log->ringkasan = "Rute " + asal + " -> " + tujuan
                       + " ditemukan pada indeks "
                       + std::to_string(kiri) + "-" + std::to_string(kanan)
                       + " (" + std::to_string(jumlahRute) + " tiket)";
        if (!tanggal.empty()) {
            log->ringkasan += ", " + std::to_string(jumlah)
                            + " tiket cocok dengan tanggal " + tanggal;
        }
        log->ringkasan += ".";
    }

    return jumlah;
}