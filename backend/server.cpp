#include <climits>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#include "httplib.h"
#include "json.hpp"
#include "ticket.h"

using json = nlohmann::json;

// ============================================================
//  DATA BERSAMA ANTAR PERMINTAAN
//  Server melayani beberapa permintaan sekaligus (multi-thread),
//  jadi semua akses ke array tiket/booking dikunci dengan mutex.
// ============================================================
static std::mutex dataMutex;

// Log eksekusi terakhir (untuk halaman Log Algoritma)
static SearchLog logPencarianTerakhir;
static SortLog logSortingTerakhir;
static bool adaLogPencarian = false;
static bool adaLogSorting = false;

// ============================================================
//  FUNGSI BANTU: kirim respons JSON dengan format yang konsisten
//    { "success": true/false, "message": "...", "data": ... }
// ============================================================
static void kirim(
    httplib::Response& res,
    int status,
    bool sukses,
    const std::string& pesan,
    const json& data
) {
    json isi = json::object();
    isi["success"] = sukses;
    isi["message"] = pesan;
    isi["data"] = data;

    res.status = status;
    // error_handler_t::replace = karakter UTF-8 yang rusak diganti, bukan membuat server error
    res.set_content(
        isi.dump(-1, ' ', false, json::error_handler_t::replace),
        "application/json; charset=utf-8"
    );
}

static void kirimGagal(httplib::Response& res, int status, const std::string& pesan) {
    kirim(res, status, false, pesan, json(nullptr));
}

// ============================================================
//  KONVERSI STRUCT -> JSON
//  Nama kunci JSON sengaja sama dengan yang dipakai frontend.
// ============================================================
static json tiketKeJson(const Ticket* t) {
    json j = json::object();
    j["kode"] = t->kode;
    j["operatorBus"] = t->namaBus;
    j["asal"] = t->asal;
    j["tujuan"] = t->tujuan;
    j["tanggal"] = t->tanggal;
    j["jam"] = t->jam;
    j["harga"] = t->harga;
    j["kursi"] = t->kursiTersedia;
    return j;
}

static json daftarItemKeJson(const std::vector<SortItem>& items) {
    json arr = json::array();

    for (const SortItem& item : items) {
        json j = json::object();
        j["kode"] = item.kode;
        j["harga"] = item.harga;
        arr.push_back(j);
    }

    return arr;
}

static json logPencarianKeJson(const SearchLog& log) {
    json steps = json::array();
    int nomor = 1;

    for (const SearchStep& s : log.steps) {
        json j = json::object();
        j["fase"] = s.fase;
        j["no"] = nomor++;

        if (s.fase == "binary") {
            j["low"] = s.low;
            j["high"] = s.high;
            j["mid"] = s.mid;
        } else {
            j["idx"] = s.mid;               // fase telusur: indeks yang diperiksa
        }

        j["kode"] = s.kode;
        j["diperiksa"] = s.rute;
        j["hasil"] = s.hasil;
        j["keputusan"] = s.keputusan;
        steps.push_back(j);
    }

    json kataKunci = json::object();
    kataKunci["asal"] = log.asal;
    kataKunci["tujuan"] = log.tujuan;
    kataKunci["tanggal"] = log.tanggal;

    json j = json::object();
    j["kataKunci"] = kataKunci;
    j["steps"] = steps;
    j["perbandingan"] = log.perbandingan;
    j["ditemukan"] = log.ditemukan;
    j["indeksAwal"] = log.indeksAwal;
    j["indeksAkhir"] = log.indeksAkhir;
    j["jumlahRute"] = log.jumlahRute;
    j["jumlahHasil"] = log.jumlahHasil;
    j["arrayTerurut"] = log.arrayTerurut;
    j["ringkasan"] = log.ringkasan;
    return j;
}

static json logSortingKeJson(const SortLog& log) {
    json rounds = json::array();

    for (const SortRound& r : log.rounds) {
        json komparasi = json::array();

        for (const CompareStep& c : r.komparasi) {
            json k = json::object();
            k["j"] = c.j;
            k["kode"] = c.kode;
            k["harga"] = c.harga;
            k["kandidatIdx"] = c.kandidatIdx;
            k["kandidatKode"] = c.kandidatKode;
            k["kandidatHarga"] = c.kandidatHarga;
            k["ganti"] = c.ganti;
            komparasi.push_back(k);
        }

        json kandidat = json::object();
        kandidat["idx"] = r.kandidatIdx;
        kandidat["kode"] = r.kandidatKode;
        kandidat["harga"] = r.kandidatHarga;

        json putaran = json::object();
        putaran["putaran"] = r.putaran;
        putaran["posisi"] = r.posisi;
        putaran["kandidat"] = kandidat;
        putaran["komparasi"] = komparasi;
        putaran["tukar"] = r.tukar;
        putaran["tukarDenganIdx"] = r.kandidatIdx;
        putaran["kodeI"] = r.kodeI;
        putaran["kodeBest"] = r.kodeBest;
        putaran["keadaan"] = daftarItemKeJson(r.keadaan);
        rounds.push_back(putaran);
    }

    json j = json::object();
    j["urut"] = log.ascending ? "asc" : "desc";
    j["n"] = log.n;
    j["awal"] = daftarItemKeJson(log.awal);
    j["rounds"] = rounds;
    j["perbandingan"] = log.perbandingan;
    j["pertukaran"] = log.pertukaran;
    j["akhir"] = daftarItemKeJson(log.akhir);
    j["ringkasan"] = log.ringkasan;
    return j;
}

static json bookingKeJson(const Booking* b) {
    const Ticket* t = cariTiketByKode(b->kodeTiket);

    json j = json::object();
    j["kodeBooking"] = b->kodeBooking;
    j["kodeTiket"] = b->kodeTiket;
    j["nama"] = b->namaPemesan;
    j["jumlahKursi"] = b->jumlahKursi;
    j["totalHarga"] = b->totalHarga;

    if (t != nullptr) {
        j["asal"] = t->asal;
        j["tujuan"] = t->tujuan;
        j["tanggal"] = t->tanggal;
        j["jam"] = t->jam;
        j["operatorBus"] = t->namaBus;
        j["hargaSatuan"] = t->harga;
    }

    return j;
}

// ============================================================
//  FUNGSI BANTU DATA (pencarian linear biasa, bukan bagian
//  algoritma yang dinilai)
// ============================================================
static bool adaDalam(const std::vector<std::string>& daftar, const std::string& nilai) {
    for (const std::string& item : daftar) {
        if (item == nilai) {
            return true;
        }
    }
    return false;
}

// Mengubah huruf besar menjadi huruf kecil (ASCII).
static std::string kecil(const std::string& teks) {
    std::string hasil = teks;

    for (char& c : hasil) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }

    return hasil;
}

// Daftar kota unik, terurut abjad (disisipkan pada posisi yang tepat).
static std::vector<std::string> daftarKota() {
    std::vector<std::string> kota;

    for (int i = 0; i < ticketCount; i++) {
        const std::string* dua[2] = {&tickets[i].asal, &tickets[i].tujuan};

        for (const std::string* nama : dua) {
            if (adaDalam(kota, *nama)) {
                continue;
            }

            std::size_t posisi = 0;
            while (posisi < kota.size() && kota[posisi] < *nama) {
                posisi++;
            }
            kota.insert(kota.begin() + static_cast<std::ptrdiff_t>(posisi), *nama);
        }
    }

    return kota;
}

// Karena array terurut menurut rute, tiket dengan rute yang sama
// berdekatan, jadi cukup satu kali lewat untuk membuat daftar rute.
static json daftarRute() {
    json rute = json::array();
    std::string asalTerakhir;
    std::string tujuanTerakhir;
    json* terakhir = nullptr;

    for (int i = 0; i < ticketCount; i++) {
        const Ticket* t = &tickets[i];

        if (terakhir != nullptr && t->asal == asalTerakhir && t->tujuan == tujuanTerakhir) {
            (*terakhir)["jumlah"] = (*terakhir)["jumlah"].get<int>() + 1;

            if (t->harga < (*terakhir)["hargaMulai"].get<int>()) {
                (*terakhir)["hargaMulai"] = t->harga;
            }
        } else {
            json baru = json::object();
            baru["asal"] = t->asal;
            baru["tujuan"] = t->tujuan;
            baru["jumlah"] = 1;
            baru["hargaMulai"] = t->harga;
            rute.push_back(baru);

            terakhir = &rute.back();
            asalTerakhir = t->asal;
            tujuanTerakhir = t->tujuan;
        }
    }

    return rute;
}

// ============================================================
//  ENDPOINT API
// ============================================================
static void daftarkanApi(httplib::Server& svr) {

    // ---------- GET /api/stats ----------
    svr.Get("/api/stats", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> kunci(dataMutex);

        int tiketTersedia = 0;
        int totalKursi = 0;
        int hargaTermurah = 0;
        std::vector<std::string> operatorBus;

        for (int i = 0; i < ticketCount; i++) {
            if (tickets[i].kursiTersedia > 0) {
                tiketTersedia++;
            }
            totalKursi += tickets[i].kursiTersedia;

            if (i == 0 || tickets[i].harga < hargaTermurah) {
                hargaTermurah = tickets[i].harga;
            }
            if (!adaDalam(operatorBus, tickets[i].namaBus)) {
                operatorBus.push_back(tickets[i].namaBus);
            }
        }

        int totalPemesanan = 0;
        for (int i = 0; i < bookingCount; i++) {
            totalPemesanan += bookings[i].totalHarga;
        }

        json data = json::object();
        data["tiketTersedia"] = tiketTersedia;
        data["totalKursi"] = totalKursi;
        data["ruteAktif"] = daftarRute().size();
        data["jumlahKota"] = daftarKota().size();
        data["operatorBus"] = operatorBus.size();
        data["pemesanan"] = bookingCount;
        data["totalPemesanan"] = totalPemesanan;
        data["hargaTermurah"] = hargaTermurah;

        kirim(res, 200, true, "OK", data);
    });

    // ---------- GET /api/cities ----------
    svr.Get("/api/cities", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> kunci(dataMutex);
        kirim(res, 200, true, "OK", json(daftarKota()));
    });

    // ---------- GET /api/routes ----------
    svr.Get("/api/routes", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> kunci(dataMutex);
        kirim(res, 200, true, "OK", daftarRute());
    });

    // ---------- GET /api/tickets  (semua tiket, urutan array utama) ----------
    svr.Get("/api/tickets", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> kunci(dataMutex);

        json arr = json::array();
        for (int i = 0; i < ticketCount; i++) {
            arr.push_back(tiketKeJson(&tickets[i]));
        }

        kirim(res, 200, true, "OK", arr);
    });

    // ---------- GET /api/tickets/search?asal=&tujuan=&tanggal=&urut=asc|desc&catat=0|1 ----------
    // Binary Search (rute) lalu Selection Sort (harga).
    // catat=0 -> hasil dikirim tetapi log terakhir tidak ditimpa.
    svr.Get("/api/tickets/search", [](const httplib::Request& req, httplib::Response& res) {
        std::string asal = req.get_param_value("asal");
        std::string tujuan = req.get_param_value("tujuan");
        std::string tanggal = req.get_param_value("tanggal");
        std::string urut = req.get_param_value("urut");
        bool catat = req.get_param_value("catat") != "0";

        if (urut.empty()) {
            urut = "asc";
        }

        if (asal.empty() || tujuan.empty()) {
            kirimGagal(res, 400, "Pilih kota asal dan kota tujuan.");
            return;
        }
        if (asal == tujuan) {
            kirimGagal(res, 400, "Kota asal dan kota tujuan tidak boleh sama.");
            return;
        }
        if (urut != "asc" && urut != "desc") {
            kirimGagal(res, 400, "Parameter urut harus asc atau desc.");
            return;
        }

        std::lock_guard<std::mutex> kunci(dataMutex);

        Ticket* hasil[MAX_TICKETS] = {};
        SearchLog logCari;
        int jumlah = cariTiketByRute(asal, tujuan, tanggal, hasil, MAX_TICKETS, &logCari);

        SortLog logUrut;
        selectionSortHarga(hasil, jumlah, urut == "asc", &logUrut);

        if (catat) {
            logPencarianTerakhir = logCari;
            logSortingTerakhir = logUrut;
            adaLogPencarian = true;
            adaLogSorting = true;
        }

        json tiket = json::array();
        for (int i = 0; i < jumlah; i++) {
            tiket.push_back(tiketKeJson(hasil[i]));
        }

        json data = json::object();
        data["tiket"] = tiket;
        data["logSearch"] = logPencarianKeJson(logCari);
        data["logSort"] = logSortingKeJson(logUrut);

        kirim(res, 200, true, "OK", data);
    });

    // ---------- GET /api/tickets/keyword?q= ----------
    // Pencarian kata kunci sederhana (pemindaian linear), dipakai kolom pencarian header.
    svr.Get("/api/tickets/keyword", [](const httplib::Request& req, httplib::Response& res) {
        std::string q = req.get_param_value("q");

        if (q.empty()) {
            kirimGagal(res, 400, "Kata kunci tidak boleh kosong.");
            return;
        }

        std::lock_guard<std::mutex> kunci(dataMutex);

        std::string cari = kecil(q);
        json tiket = json::array();

        for (int i = 0; i < ticketCount; i++) {
            const Ticket* t = &tickets[i];
            std::string gabung = kecil(t->kode + " " + t->namaBus + " " + t->asal + " " + t->tujuan);

            if (gabung.find(cari) != std::string::npos) {
                tiket.push_back(tiketKeJson(t));
            }
        }

        json data = json::object();
        data["tiket"] = tiket;
        data["kataKunci"] = q;
        kirim(res, 200, true, "OK", data);
    });

    // ---------- GET /api/tickets/{kode} ----------
    // Didaftarkan SETELAH search dan keyword supaya tidak menimpa keduanya.
    svr.Get(R"(/api/tickets/([A-Za-z0-9_-]+))", [](const httplib::Request& req, httplib::Response& res) {
        std::string kode = req.matches[1];

        std::lock_guard<std::mutex> kunci(dataMutex);

        const Ticket* t = cariTiketByKode(kode);
        if (t == nullptr) {
            kirimGagal(res, 404, "Tiket " + kode + " tidak ditemukan.");
            return;
        }

        kirim(res, 200, true, "OK", tiketKeJson(t));
    });

    // ---------- POST /api/bookings   body: {"kode":"..","nama":"..","jumlah":2} ----------
    svr.Post("/api/bookings", [](const httplib::Request& req, httplib::Response& res) {
        json body = json::parse(req.body, nullptr, false);   // false = tanpa exception

        if (body.is_discarded() || !body.is_object()) {
            kirimGagal(res, 400, "Body harus berupa JSON yang valid.");
            return;
        }
        if (!body.contains("kode") || !body["kode"].is_string()) {
            kirimGagal(res, 400, "Kode tiket wajib diisi.");
            return;
        }
        if (!body.contains("nama") || !body["nama"].is_string()) {
            kirimGagal(res, 400, "Nama pemesan wajib diisi.");
            return;
        }
        if (!body.contains("jumlah") || !body["jumlah"].is_number_integer()) {
            kirimGagal(res, 400, "Jumlah kursi harus bilangan bulat minimal 1.");
            return;
        }

        std::string kode = body["kode"].get<std::string>();
        std::string nama = body["nama"].get<std::string>();

        long long jumlahBesar;
        if (body["jumlah"].is_number_unsigned()) {
            unsigned long long u = body["jumlah"].get<unsigned long long>();
            jumlahBesar = (u > static_cast<unsigned long long>(INT_MAX)) ? INT_MAX : static_cast<long long>(u);
        } else {
            jumlahBesar = body["jumlah"].get<long long>();
        }
        if (jumlahBesar > INT_MAX) {
            jumlahBesar = INT_MAX;
        }
        if (jumlahBesar < 1) {
            kirimGagal(res, 400, "Jumlah kursi harus bilangan bulat minimal 1.");
            return;
        }

        std::lock_guard<std::mutex> kunci(dataMutex);

        if (cariTiketByKode(kode) == nullptr) {
            kirimGagal(res, 404, "Tiket " + kode + " tidak ditemukan.");
            return;
        }

        BookingResult hasil = buatBooking(kode, nama, static_cast<int>(jumlahBesar));

        if (!hasil.sukses) {
            kirimGagal(res, 400, hasil.pesan);
            return;
        }

        kirim(res, 201, true, hasil.pesan, bookingKeJson(&hasil.data));
    });

    // ---------- GET /api/bookings ----------
    svr.Get("/api/bookings", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> kunci(dataMutex);

        json arr = json::array();
        for (int i = 0; i < bookingCount; i++) {
            arr.push_back(bookingKeJson(&bookings[i]));
        }

        kirim(res, 200, true, "OK", arr);
    });

    // ---------- GET /api/logs ----------
    svr.Get("/api/logs", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> kunci(dataMutex);

        json data = json::object();
        data["sorting"] = adaLogSorting ? logSortingKeJson(logSortingTerakhir) : json(nullptr);
        data["searching"] = adaLogPencarian ? logPencarianKeJson(logPencarianTerakhir) : json(nullptr);

        kirim(res, 200, true, "OK", data);
    });

    // ---------- DELETE /api/logs ----------
    svr.Delete("/api/logs", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> kunci(dataMutex);

        logPencarianTerakhir = SearchLog();
        logSortingTerakhir = SortLog();
        adaLogPencarian = false;
        adaLogSorting = false;

        kirim(res, 200, true, "Log dihapus.", json(nullptr));
    });

    // ---------- Alamat /api/... lain yang tidak dikenal ----------
    auto tidakDikenal = [](const httplib::Request& req, httplib::Response& res) {
        kirimGagal(res, 404, "Endpoint " + req.method + " " + req.path + " tidak ditemukan.");
    };
    svr.Get(R"(/api/.*)", tidakDikenal);
    svr.Post(R"(/api/.*)", tidakDikenal);
    svr.Delete(R"(/api/.*)", tidakDikenal);

    // ---------- Kesalahan tak terduga -> JSON 500 ----------
    svr.set_exception_handler([](const httplib::Request&, httplib::Response& res, std::exception_ptr) {
        kirimGagal(res, 500, "Terjadi kesalahan pada server.");
    });

    // ---------- Catat setiap permintaan API di terminal ----------
    svr.set_logger([](const httplib::Request& req, const httplib::Response& res) {
        if (req.path.rfind("/api/", 0) == 0) {
            std::cout << req.method << " " << req.path << " -> " << res.status << std::endl;
        }
    });
}

// ============================================================
//  Mencari folder frontend (dijalankan dari folder BusKita
//  atau dari dalam folder backend).
// ============================================================
static std::string cariFolderFrontend() {
    const std::string kandidat[] = {"frontend", "../frontend"};

    for (const std::string& folder : kandidat) {
        std::ifstream berkas(folder + "/index.html");
        if (berkas.good()) {
            return folder;
        }
    }

    return "";
}

int main(int argc, char* argv[]) {
    int port = 8080;

    if (argc >= 2) {
        port = std::atoi(argv[1]);
        if (port < 1 || port > 65535) {
            std::cerr << "Port tidak valid. Contoh: buskita.exe 8080\n";
            return 1;
        }
    }

    httplib::Server svr;
    daftarkanApi(svr);

    std::string folder = cariFolderFrontend();
    if (folder.empty()) {
        std::cout << "PERINGATAN: frontend/index.html tidak ditemukan.\n"
                  << "Jalankan dari folder BusKita agar halaman web ikut dilayani.\n"
                  << "API tetap berjalan.\n\n";
    } else if (!svr.set_mount_point("/", folder)) {
        std::cerr << "Gagal memasang folder frontend: " << folder << '\n';
        return 1;
    }

    std::cout << "================================\n";
    std::cout << "        BUSKITA SERVER\n";
    std::cout << "================================\n";
    std::cout << "Tiket di array   : " << ticketCount << " dari " << MAX_TICKETS << '\n';
    std::cout << "Array terurut    : " << (arrayTerurutByRute() ? "ya" : "TIDAK") << '\n';
    std::cout << "Buka di browser  : http://127.0.0.1:" << port << '\n';
    std::cout << "Hentikan server  : tekan Ctrl+C\n" << std::endl;

    if (!svr.listen("127.0.0.1", port)) {
        std::cerr << "Gagal menjalankan server di port " << port
                  << ". Port mungkin sedang dipakai program lain.\n"
                  << "Coba port lain, misalnya: buskita.exe 8081\n";
        return 1;
    }

    return 0;
}