#include "ticket.h"

// ============================================================
//  Fungsi bantu: mencatat isi array hasil[] (kode + harga)
//  pada saat ini. Dipakai untuk log "keadaan array".
//  "static" = fungsi ini hanya bisa dipakai di file sorting.cpp.
// ============================================================
static std::vector<SortItem> fotoArray(Ticket* hasil[], int jumlah) {
    std::vector<SortItem> foto;

    for (int i = 0; i < jumlah; i++) {
        foto.push_back(SortItem{hasil[i]->kode, hasil[i]->harga});
    }

    return foto;
}

// ============================================================
//  SELECTION SORT berdasarkan harga.
//
//  - hasil[] adalah array POINTER ke tiket (hasil pencarian).
//    Yang ditukar hanya pointer-nya, bukan isi struct, sehingga
//    array utama tickets[] tidak berubah urutannya.
//  - ascending = true  -> termurah ke termahal
//    ascending = false -> termahal ke termurah
//  - Jika log != nullptr, setiap langkah dicatat ke log.
//
//  Cara kerja: pada setiap putaran i, cari harga terkecil
//  (atau terbesar) dari posisi i sampai akhir, lalu tukar ke
//  posisi i. Setelah putaran ke-i, posisi 0..i sudah final.
// ============================================================
void selectionSortHarga(
    Ticket* hasil[],
    int jumlah,
    bool ascending,
    SortLog* log
) {
    int totalPerbandingan = 0;
    int totalPertukaran = 0;

    if (log != nullptr) {
        log->ascending = ascending;
        log->n = jumlah;
        log->awal = fotoArray(hasil, jumlah);
    }

    for (int i = 0; i < jumlah - 1; i++) {
        int kandidat = i;                       // indeks harga min/maks sementara
        std::vector<CompareStep> daftarBanding;

        for (int j = i + 1; j < jumlah; j++) {
            totalPerbandingan++;

            bool lebihBaik = ascending
                ? hasil[j]->harga < hasil[kandidat]->harga
                : hasil[j]->harga > hasil[kandidat]->harga;

            if (log != nullptr) {
                CompareStep langkah;
                langkah.j = j;
                langkah.kode = hasil[j]->kode;
                langkah.harga = hasil[j]->harga;
                langkah.kandidatIdx = kandidat;
                langkah.kandidatKode = hasil[kandidat]->kode;
                langkah.kandidatHarga = hasil[kandidat]->harga;
                langkah.ganti = lebihBaik;
                daftarBanding.push_back(langkah);
            }

            if (lebihBaik) {
                kandidat = j;
            }
        }

        bool tukar = (kandidat != i);

        // Simpan informasi SEBELUM pertukaran
        std::string kodeI = hasil[i]->kode;
        std::string kodeKandidat = hasil[kandidat]->kode;
        int hargaKandidat = hasil[kandidat]->harga;

        if (tukar) {
            Ticket* sementara = hasil[i];       // tukar dua POINTER
            hasil[i] = hasil[kandidat];
            hasil[kandidat] = sementara;
            totalPertukaran++;
        }

        if (log != nullptr) {
            SortRound putaran;
            putaran.putaran = i + 1;
            putaran.posisi = i;
            putaran.kandidatIdx = kandidat;
            putaran.kandidatKode = kodeKandidat;
            putaran.kandidatHarga = hargaKandidat;
            putaran.komparasi = daftarBanding;
            putaran.tukar = tukar;
            putaran.kodeI = kodeI;
            putaran.kodeBest = kodeKandidat;
            putaran.keadaan = fotoArray(hasil, jumlah);
            log->rounds.push_back(putaran);
        }
    }

    if (log != nullptr) {
        log->akhir = fotoArray(hasil, jumlah);
        log->perbandingan = totalPerbandingan;
        log->pertukaran = totalPertukaran;

        if (jumlah < 2) {
            log->ringkasan = "Hanya " + std::to_string(jumlah)
                           + " data, pengurutan tidak diperlukan.";
        } else {
            log->ringkasan = std::string(ascending ? "Termurah ke termahal"
                                                   : "Termahal ke termurah")
                           + ": " + std::to_string(jumlah) + " data, "
                           + std::to_string(totalPerbandingan) + " perbandingan, "
                           + std::to_string(totalPertukaran) + " pertukaran.";
        }
    }
}