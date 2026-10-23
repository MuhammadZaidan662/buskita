'use strict';
/* ==========================================================
   BusKita - app.js  (FRONTEND TERHUBUNG KE SERVER C++)

   Struktur file:
     1. Helper kecil
     2. Lapisan api : satu-satunya pintu data untuk UI.
        Setiap fungsi memanggil server C++ lewat fetch('/api/...').
        Array, Pointer, Struct, Selection Sort, dan Binary Search
        TIDAK ada di file ini: semuanya dijalankan oleh C++.
     3. UI : navigasi, render halaman, form
   ========================================================== */


/* ==========================================================
   1. HELPER
   ========================================================== */
const $  = (sel, root = document) => root.querySelector(sel);
const $$ = (sel, root = document) => Array.from(root.querySelectorAll(sel));

const esc = (s) => String(s).replace(/[&<>"']/g, (c) => (
  { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]
));
const rp = (n) => 'Rp' + Number(n).toLocaleString('id-ID');
const BULAN = ['Jan', 'Feb', 'Mar', 'Apr', 'Mei', 'Jun', 'Jul', 'Agu', 'Sep', 'Okt', 'Nov', 'Des'];
const tgl = (iso) => {
  const p = String(iso).split('-');
  return p.length === 3 ? `${+p[2]} ${BULAN[+p[1] - 1]} ${p[0]}` : iso;
};
const icon = (name, cls = '') => `<svg class="ic ${cls}"><use href="#i-${name}"/></svg>`;



/* ==========================================================
   2. LAPISAN API  (berbicara dengan server C++)
   Semua UI hanya memanggil api.*, yang selalu mengembalikan
   { success, message, data } - bentuk yang sama dengan jawaban server.
   ========================================================== */

// Teks dari C++ memakai "->" dan tanggal ISO; di layar dibuat lebih enak dibaca.
function rapikan(nilai, kunci) {
  if (Array.isArray(nilai)) return nilai.map((x) => rapikan(x, kunci));
  if (nilai && typeof nilai === 'object') {
    const o = {};
    for (const k of Object.keys(nilai)) o[k] = rapikan(nilai[k], k);
    return o;
  }
  if (typeof nilai === 'string') {
    let s = nilai.split(' -> ').join(' → ');
    if (kunci === 'ringkasan') s = s.replace(/(\d{4})-(\d{2})-(\d{2})/g, (m) => tgl(m));
    return s;
  }
  return nilai;
}

// Banner tetap jika server C++ tidak bisa dihubungi
function tampilkanServerMati() {
  let el = $('#serverDown');
  if (!el) {
    el = document.createElement('div');
    el.id = 'serverDown';
    el.className = 'server-down';
    el.setAttribute('role', 'alert');
    el.innerHTML = '<b>Server BusKita tidak dapat dihubungi.</b> ' +
      'Jalankan <code>backend\\buskita.exe</code> dari folder BusKita, lalu buka halaman ini lewat ' +
      '<code>http://127.0.0.1:8080</code> (bukan dengan klik dua kali index.html).';
    document.body.prepend(el);
  }
  el.hidden = false;
}

async function request(method, url, body) {
  let res;
  try {
    res = await fetch(url, {
      method,
      headers: body ? { 'Content-Type': 'application/json' } : undefined,
      body: body ? JSON.stringify(body) : undefined,
    });
  } catch (e) {
    tampilkanServerMati();
    throw new Error('Server tidak dapat dihubungi');
  }
  const bar = $('#serverDown'); if (bar) bar.hidden = true;

  let json;
  try { json = await res.json(); }                  // server selalu membalas JSON, juga saat 400/404
  catch (e) { return { success: false, message: 'Jawaban server tidak dapat dibaca.', data: null }; }
  return rapikan(json);
}

const qs = (obj) => Object.keys(obj)
  .filter((k) => obj[k] !== undefined && obj[k] !== null && obj[k] !== '')
  .map((k) => encodeURIComponent(k) + '=' + encodeURIComponent(obj[k])).join('&');

const api = {
  stats()  { return request('GET', '/api/stats'); },
  cities() { return request('GET', '/api/cities'); },
  routes() { return request('GET', '/api/routes'); },
  tickets() { return request('GET', '/api/tickets'); },
  detail(kode) { return request('GET', '/api/tickets/' + encodeURIComponent(kode)); },

  // Binary Search (rute) lalu Selection Sort (harga) dijalankan di C++.
  // record=false -> server tidak menimpa log terakhir (catat=0).
  search({ asal, tujuan, tanggal, urut }, { record = true } = {}) {
    return request('GET', '/api/tickets/search?' + qs({ asal, tujuan, tanggal, urut, catat: record ? '1' : '0' }));
  },

  keyword(q) { return request('GET', '/api/tickets/keyword?' + qs({ q })); },

  book({ kode, nama, jumlah }) { return request('POST', '/api/bookings', { kode, nama, jumlah }); },
  bookings() { return request('GET', '/api/bookings'); },

  logs() { return request('GET', '/api/logs'); },
  resetLogs() { return request('DELETE', '/api/logs'); },
};

// Kesalahan jaringan yang tidak tertangkap: banner sudah tampil, cukup diam
window.addEventListener('unhandledrejection', (e) => {
  if (e.reason && e.reason.message === 'Server tidak dapat dihubungi') e.preventDefault();
});


/* ==========================================================
   3. UI
   ========================================================== */
const ui = {
  urut: 'asc',
  mode: null,            // 'route' | 'keyword' | null
  params: null,          // parameter pencarian terakhir
  pesanKode: '',
  lastBooking: null,
  carIdx: 0,
  carTimer: null,
};

/* ---------- Toast ---------- */
let toastTimer;
function toast(msg) {
  const el = $('#toast');
  el.textContent = msg;
  el.hidden = false;
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => { el.hidden = true; }, 2800);
}

function showMsg(el, text, good = false) {
  if (!text) { el.hidden = true; el.textContent = ''; return; }
  el.hidden = false;
  el.textContent = text;
  el.classList.toggle('ok', good);
}

/* ---------- Navigasi ---------- */
const VIEWS = {
  'beranda': 'view-beranda', 'cari': 'view-cari', 'rute': 'view-rute', 'pesan': 'view-pesan',
  'riwayat': 'view-riwayat', 'log-sorting': 'view-log', 'log-searching': 'view-log', 'tentang': 'view-tentang',
};

function goto(name) {
  if (location.hash === '#' + name) route();
  else location.hash = name;
}

function route() {
  let name = location.hash.replace('#', '') || 'beranda';
  if (!VIEWS[name]) name = 'beranda';

  $$('.view').forEach((v) => { v.hidden = v.id !== VIEWS[name]; });
  $$('.nav-item[data-view], .nav-sub[data-view]').forEach((a) => {
    const aktif = a.dataset.view === name;
    a.classList.toggle('active', aktif);
    if (aktif) a.setAttribute('aria-current', 'page'); else a.removeAttribute('aria-current');
  });
  const logAktif = name.startsWith('log-');
  $('#grpLog').classList.toggle('active', logAktif);
  if (logAktif) setLogGroup(true);

  closeMenu();
  window.scrollTo({ top: 0 });

  if (name === 'beranda') renderStats();
  if (name === 'rute') renderRute();
  if (name === 'pesan') renderPesan();
  if (name === 'riwayat') renderRiwayat();
  if (logAktif) renderLogs(name);
  document.title = 'BusKita - ' + ({ beranda: 'Beranda', cari: 'Cari Tiket', rute: 'Rute Bus', pesan: 'Pemesanan',
    riwayat: 'Riwayat Booking', 'log-sorting': 'Log Sorting', 'log-searching': 'Log Searching', tentang: 'Tentang Aplikasi' }[name]);
}

function setLogGroup(open) {
  $('#grpLog').setAttribute('aria-expanded', String(open));
  $('#subLog').hidden = !open;
}

/* ---------- Menu mobile ---------- */
function openMenu() { $('#sidebar').classList.add('open'); $('#scrim').hidden = false; }
function closeMenu() { $('#sidebar').classList.remove('open'); $('#scrim').hidden = true; }

/* ---------- Carousel ---------- */
function busArt() {
  return `<svg viewBox="0 0 280 150" role="img" aria-label="Ilustrasi bus">
    <ellipse cx="140" cy="136" rx="120" ry="7" fill="rgba(0,0,0,.2)"/>
    <rect x="22" y="26" width="236" height="94" rx="20" fill="#fff"/>
    <rect x="22" y="86" width="236" height="14" fill="#FFD2B3"/>
    <rect x="38" y="40" width="36" height="34" rx="7" fill="#BFDBFE"/>
    <rect x="82" y="40" width="36" height="34" rx="7" fill="#BFDBFE"/>
    <rect x="126" y="40" width="36" height="34" rx="7" fill="#BFDBFE"/>
    <rect x="170" y="40" width="36" height="34" rx="7" fill="#BFDBFE"/>
    <rect x="216" y="40" width="30" height="46" rx="8" fill="#93C5FD"/>
    <circle cx="250" cy="106" r="4" fill="#FDE68A"/>
    <circle cx="82" cy="122" r="17" fill="#1F2937"/><circle cx="82" cy="122" r="7" fill="#E5E7EB"/>
    <circle cx="200" cy="122" r="17" fill="#1F2937"/><circle cx="200" cy="122" r="7" fill="#E5E7EB"/>
  </svg>`;
}
function ticketArt(harga) {
  return `<svg viewBox="0 0 280 150" role="img" aria-label="Ilustrasi tiket">
    <path d="M24 28h232a8 8 0 0 1 8 8v22a14 14 0 0 0 0 28v28a8 8 0 0 1-8 8H24a8 8 0 0 1-8-8V86a14 14 0 0 0 0-28V36a8 8 0 0 1 8-8z" fill="#fff"/>
    <line x1="192" y1="36" x2="192" y2="122" stroke="#CBD5E1" stroke-width="2" stroke-dasharray="5 5"/>
    <text x="40" y="62" font-size="12" fill="#64748B" font-family="Poppins,sans-serif">Mulai dari</text>
    <text x="40" y="92" font-size="26" font-weight="700" fill="#1E40AF" font-family="Poppins,sans-serif">${rp(harga)}</text>
    <text x="40" y="114" font-size="11" fill="#64748B" font-family="Poppins,sans-serif">per kursi</text>
    <circle cx="228" cy="79" r="20" fill="#DBEAFE"/>
    <path d="M220 79l6 6 12-13" stroke="#2563EB" stroke-width="4" fill="none" stroke-linecap="round" stroke-linejoin="round"/>
  </svg>`;
}
function routeArt() {
  return `<svg viewBox="0 0 280 150" role="img" aria-label="Ilustrasi rute perjalanan">
    <path d="M50 108 C 100 108, 90 50, 140 60 S 200 100, 232 44" fill="none" stroke="#fff" stroke-width="5" stroke-dasharray="2 11" stroke-linecap="round"/>
    <g transform="translate(34 92)"><path d="M16 36C16 36 0 22 0 14a16 16 0 0 1 32 0c0 8-16 22-16 22z" fill="#fff"/><circle cx="16" cy="14" r="6" fill="#F26B21"/></g>
    <g transform="translate(216 22)"><path d="M16 36C16 36 0 22 0 14a16 16 0 0 1 32 0c0 8-16 22-16 22z" fill="#fff"/><circle cx="16" cy="14" r="6" fill="#2563EB"/></g>
    <rect x="104" y="64" width="64" height="34" rx="10" fill="#fff"/>
    <rect x="112" y="72" width="14" height="12" rx="3" fill="#BFDBFE"/><rect x="130" y="72" width="14" height="12" rx="3" fill="#BFDBFE"/><rect x="148" y="72" width="14" height="12" rx="3" fill="#BFDBFE"/>
    <circle cx="120" cy="100" r="6" fill="#1F2937"/><circle cx="152" cy="100" r="6" fill="#1F2937"/>
  </svg>`;
}

async function initCarousel() {
  const st = (await api.stats()).data;
  const slides = [
    { cls: 's1', tag: 'Antarkota', judul: 'Pilihan bus antarkota untuk perjalananmu',
      isi: `${st.ruteAktif} rute aktif menghubungkan ${st.jumlahKota} kota dengan ${st.operatorBus} operator bus.`,
      cta: 'Lihat rute', to: 'rute', art: busArt() },
    { cls: 's2', tag: 'Harga bersahabat', judul: 'Temukan tiket dengan harga yang sesuai',
      isi: 'Urutkan hasil dari harga termurah atau termahal dalam sekali klik.',
      cta: 'Cari tiket', to: 'cari', art: ticketArt(st.hargaTermurah) },
    { cls: 's3', tag: 'Mudah', judul: 'Pesan perjalanan dengan mudah melalui BusKita',
      isi: 'Pilih tiket, isi nama, dan booking langsung tercatat.',
      cta: 'Pesan sekarang', to: 'pesan', art: routeArt() },
  ];
  $('#carTrack').innerHTML = slides.map((s, i) => `
    <article class="slide ${s.cls}" role="group" aria-roledescription="slide" aria-label="${i + 1} dari ${slides.length}">
      <div class="slide-copy">
        <span class="slide-tag">${s.tag}</span>
        <h2>${esc(s.judul)}</h2>
        <p>${esc(s.isi)}</p>
        <button class="btn" type="button" data-go="${s.to}">${esc(s.cta)} ${icon('arrow')}</button>
      </div>
      <div class="slide-art">${s.art}</div>
    </article>`).join('');
  $('#carDots').innerHTML = slides.map((_, i) => `<button type="button" data-dot="${i}" aria-label="Banner ${i + 1}"></button>`).join('');
  ui.carCount = slides.length;
  setSlide(0);
  startAuto();
}
function setSlide(i) {
  ui.carIdx = (i + ui.carCount) % ui.carCount;
  $('#carTrack').style.transform = `translateX(-${ui.carIdx * 100}%)`;
  $$('#carDots button').forEach((b, k) => b.classList.toggle('on', k === ui.carIdx));
}
function startAuto() {
  stopAuto();
  if (window.matchMedia('(prefers-reduced-motion: reduce)').matches) return;
  ui.carTimer = setInterval(() => setSlide(ui.carIdx + 1), 5500);
}
function stopAuto() { clearInterval(ui.carTimer); }

/* ---------- Statistik ---------- */
async function renderStats() {
  const s = (await api.stats()).data;
  const kartu = [
    { tone: 't-orange', ic: 'ticket',    label: 'Tiket Tersedia',      num: s.tiketTersedia, sub: `${s.totalKursi.toLocaleString('id-ID')} kursi tersisa` },
    { tone: 't-blue',   ic: 'route',     label: 'Rute Aktif',          num: s.ruteAktif,     sub: `menghubungkan ${s.jumlahKota} kota` },
    { tone: 't-green',  ic: 'briefcase', label: 'Operator Bus',        num: s.operatorBus,   sub: 'operator terdaftar' },
    { tone: 't-purple', ic: 'receipt',   label: 'Pemesanan Sesi Ini',  num: s.pemesanan,     sub: s.pemesanan ? `total ${rp(s.totalPemesanan)}` : 'belum ada pemesanan' },
  ];
  $('#statsGrid').innerHTML = kartu.map((k) => `
    <div class="stat">
      <span class="chip ${k.tone}">${icon(k.ic)}${k.label}</span>
      <div class="num">${k.num}</div>
      <div class="sub">${esc(k.sub)}</div>
    </div>`).join('');
  const badge = $('#badgeRiwayat');
  badge.hidden = s.pemesanan === 0;
  badge.textContent = s.pemesanan;
}

/* ---------- Select kota ---------- */
async function fillCities() {
  const kota = (await api.cities()).data;
  const opts = '<option value="">Pilih kota...</option>' + kota.map((k) => `<option value="${esc(k)}">${esc(k)}</option>`).join('');
  ['qAsal', 'qTujuan', 'cAsal', 'cTujuan'].forEach((id) => { $('#' + id).innerHTML = opts; });
}

/* ---------- Pencarian cepat (Beranda) ---------- */
async function submitQuick(e) {
  e.preventDefault();
  const asal = $('#qAsal').value, tujuan = $('#qTujuan').value, tanggal = $('#qTanggal').value;
  if (!asal || !tujuan) return showMsg($('#quickMsg'), 'Pilih kota asal dan kota tujuan.');
  if (asal === tujuan) return showMsg($('#quickMsg'), 'Kota asal dan kota tujuan tidak boleh sama.');
  showMsg($('#quickMsg'), '');
  $('#cAsal').value = asal; $('#cTujuan').value = tujuan; $('#cTanggal').value = tanggal;
  goto('cari');
  await runSearch();
}

/* ---------- Cari Tiket ---------- */
async function runSearch(opts = {}) {
  const params = {
    asal: $('#cAsal').value, tujuan: $('#cTujuan').value,
    tanggal: $('#cTanggal').value, urut: $('#cUrut').value,
  };
  const res = await api.search(params, { record: opts.record !== false });
  if (!res.success) {
    showMsg($('#cariMsg'), res.message);
    return;
  }
  showMsg($('#cariMsg'), '');
  ui.mode = 'route';
  ui.params = params;
  ui.urut = params.urut;
  ui.res = res.data;
  renderResults();
}

async function runKeyword(q) {
  const res = await api.keyword(q);
  ui.mode = 'keyword';
  ui.res = { tiket: res.data.tiket, kataKunci: q };
  showMsg($('#cariMsg'), '');
  renderResults();
}

function ticketCard(t) {
  const habis = t.kursi <= 0;
  const seatCls = habis ? 'zero' : (t.kursi <= 7 ? 'low' : '');
  const seatTxt = habis ? 'Kursi habis' : `${t.kursi} kursi tersedia`;
  return `
  <article class="ticket">
    <div>
      <div class="t-top">
        <span class="t-code">${esc(t.kode)}</span>
        <span class="t-op">${icon('bus')}${esc(t.operatorBus)}</span>
      </div>
      <div class="t-route"><span>${esc(t.asal)}</span>${icon('arrow')}<span>${esc(t.tujuan)}</span></div>
      <div class="t-meta">
        <span>${icon('calendar')}${tgl(t.tanggal)}</span>
        <span>${icon('history')}${esc(t.jam)} WIB</span>
      </div>
    </div>
    <div class="t-side">
      <div class="t-price">${rp(t.harga)} <small>/kursi</small></div>
      <span class="seats ${seatCls}">${seatTxt}</span>
      <div class="t-actions">
        <button class="btn btn-ghost btn-sm" type="button" data-act="detail" data-kode="${esc(t.kode)}">Detail</button>
        <button class="btn btn-primary btn-sm" type="button" data-act="pesan" data-kode="${esc(t.kode)}" ${habis ? 'disabled' : ''}>Pesan Tiket</button>
      </div>
    </div>
  </article>`;
}

function renderResults() {
  const r = ui.res;
  if (!r) return;
  const bar = $('#resultBar'), list = $('#resultList'), empty = $('#resultEmpty');
  bar.hidden = false;

  const adaSeg = ui.mode === 'route';
  $('#segUrut').hidden = !adaSeg;
  $$('#segUrut button').forEach((b) => b.classList.toggle('on', b.dataset.urut === ui.urut));

  const n = r.tiket.length;
  if (ui.mode === 'route') {
    const p = ui.params;
    const info = `${n} tiket ditemukan`;
    $('#resultInfo').innerHTML = `${info}<small>${esc(p.asal)} → ${esc(p.tujuan)}` +
      (p.tanggal ? ` · ${tgl(p.tanggal)}` : ' · semua tanggal') +
      ` · harga ${p.urut === 'desc' ? 'termahal' : 'termurah'} lebih dulu</small>`;
  } else {
    $('#resultInfo').innerHTML = `${n} tiket untuk kata kunci “${esc(r.kataKunci)}”` +
      `<small>Pencarian kata kunci (pemindaian linear), bukan Binary Search.</small>`;
  }

  list.innerHTML = r.tiket.map(ticketCard).join('');

  if (n === 0) {
    empty.hidden = false;
    empty.className = 'empty card';
    let pesan;
    if (ui.mode === 'keyword') {
      pesan = `Tidak ada tiket yang cocok dengan “${esc(r.kataKunci)}”. Coba nama kota, kode tiket, atau nama operator.`;
    } else if (!r.logSearch.ditemukan) {
      pesan = `Rute ${esc(ui.params.asal)} → ${esc(ui.params.tujuan)} belum tersedia. Lihat daftar rute di menu <a href="#rute"><u>Rute Bus</u></a>.`;
    } else {
      pesan = `Rute tersedia (${r.logSearch.jumlahRute} tiket), tetapi tidak ada yang berangkat pada ${tgl(ui.params.tanggal)}. Kosongkan tanggal untuk melihat semua jadwal.`;
    }
    empty.innerHTML = `${icon('search', 'big')}<p>${pesan}</p>`;
  } else {
    empty.hidden = true;
  }
}

/* ---------- Detail (modal) ---------- */
let lastFocus = null;
async function openDetail(kode) {
  const res = await api.detail(kode);
  if (!res.success) return toast(res.message);
  const t = res.data;
  const habis = t.kursi <= 0;
  $('#modalBody').innerHTML = `
    <span class="t-code">${esc(t.kode)}</span>
    <h2 id="modalTitle">${esc(t.asal)} → ${esc(t.tujuan)}</h2>
    <div class="t-op">${icon('bus')}${esc(t.operatorBus)}</div>
    <dl class="kv">
      <dt>Tanggal berangkat</dt><dd>${tgl(t.tanggal)}</dd>
      <dt>Jam berangkat</dt><dd>${esc(t.jam)} WIB</dd>
      <dt>Kursi tersedia</dt><dd>${t.kursi}</dd>
      <dt>Harga per kursi</dt><dd class="big">${rp(t.harga)}</dd>
    </dl>
    <div class="t-actions">
      <button class="btn btn-ghost" type="button" data-act="tutup">Tutup</button>
      <button class="btn btn-primary" type="button" data-act="pesan" data-kode="${esc(t.kode)}" ${habis ? 'disabled' : ''}>Pesan Tiket</button>
    </div>`;
  lastFocus = document.activeElement;
  $('#modal').hidden = false;
  $('#modalClose').focus();
}
function closeModal() {
  if ($('#modal').hidden) return;
  $('#modal').hidden = true;
  if (lastFocus && lastFocus.focus) lastFocus.focus();
}

/* ---------- Rute ---------- */
async function renderRute() {
  const rute = (await api.routes()).data;
  $('#tblRute tbody').innerHTML = rute.map((r, i) => `
    <tr>
      <td>${i + 1}</td>
      <td><b>${esc(r.asal)}</b> → <b>${esc(r.tujuan)}</b></td>
      <td class="num">${r.jumlah}</td>
      <td class="num">${rp(r.hargaMulai)}</td>
      <td><button class="btn btn-blue btn-sm" type="button" data-act="lihat-rute" data-asal="${esc(r.asal)}" data-tujuan="${esc(r.tujuan)}">Lihat tiket</button></td>
    </tr>`).join('');
}

/* ---------- Pemesanan ---------- */
async function renderPesan() {
  const list = (await api.tickets()).data;
  const sel = $('#bTiket');
  const keep = ui.pesanKode || sel.value;
  sel.innerHTML = '<option value="">Pilih tiket...</option>' + list.map((t) =>
    `<option value="${esc(t.kode)}" ${t.kursi <= 0 ? 'disabled' : ''}>${esc(t.kode)} · ${esc(t.asal)} → ${esc(t.tujuan)} · ${tgl(t.tanggal)} ${esc(t.jam)} · ${rp(t.harga)} (sisa ${t.kursi})</option>`
  ).join('');
  sel.value = keep;
  if (sel.value !== keep) sel.value = '';
  ui.pesanKode = '';
  await updateBookCalc();
}

async function updateBookCalc() {
  const kode = $('#bTiket').value;
  const box = $('#bookSummary');
  if (!kode) {
    $('#bHarga').value = ''; $('#bTotal').value = '';
    if (!ui.lastBooking) {
      box.innerHTML = `<div class="empty">${icon('ticket', 'big')}<p>Pilih tiket untuk melihat ringkasannya di sini.</p></div>`;
    }
    return;
  }
  const t = (await api.detail(kode)).data;
  const jml = parseInt($('#bJumlah').value, 10);
  $('#bHarga').value = rp(t.harga);
  $('#bTotal').value = Number.isInteger(jml) && jml > 0 ? rp(jml * t.harga) : '-';
  if (ui.lastBooking && ui.lastBooking.kodeTiket !== kode) ui.lastBooking = null;
  if (!ui.lastBooking) {
    box.innerHTML = `
      <h3>Ringkasan tiket</h3>
      <dl class="kv">
        <dt>Kode</dt><dd>${esc(t.kode)}</dd>
        <dt>Operator</dt><dd>${esc(t.operatorBus)}</dd>
        <dt>Rute</dt><dd>${esc(t.asal)} → ${esc(t.tujuan)}</dd>
        <dt>Berangkat</dt><dd>${tgl(t.tanggal)}, ${esc(t.jam)}</dd>
        <dt>Kursi tersisa</dt><dd>${t.kursi}</dd>
        <dt>Harga per kursi</dt><dd>${rp(t.harga)}</dd>
      </dl>`;
  }
}

function showBookingSuccess(b) {
  $('#bookSummary').innerHTML = `
    <div class="success">
      <div class="ok-ic">${icon('check')}</div>
      <h3>Pemesanan berhasil</h3>
      <div class="code">${esc(b.kodeBooking)}</div>
      <dl class="kv">
        <dt>Pemesan</dt><dd>${esc(b.nama)}</dd>
        <dt>Tiket</dt><dd>${esc(b.kodeTiket)} · ${esc(b.operatorBus)}</dd>
        <dt>Rute</dt><dd>${esc(b.asal)} → ${esc(b.tujuan)}</dd>
        <dt>Berangkat</dt><dd>${tgl(b.tanggal)}, ${esc(b.jam)}</dd>
        <dt>Jumlah kursi</dt><dd>${b.jumlahKursi}</dd>
      </dl>
      <hr>
      <dl class="kv"><dt>Total</dt><dd class="big">${rp(b.totalHarga)}</dd></dl>
      <div class="t-actions" style="justify-content:center;margin-top:16px">
        <a class="btn btn-ghost btn-sm" href="#riwayat">Lihat riwayat</a>
        <a class="btn btn-primary btn-sm" href="#cari">Cari tiket lain</a>
      </div>
    </div>`;
}

async function submitBooking(e) {
  e.preventDefault();
  const kode = $('#bTiket').value;
  const nama = $('#bNama').value;
  const jumlah = Number($('#bJumlah').value);
  const msg = $('#bookMsg');
  if (!kode) return showMsg(msg, 'Pilih kode tiket terlebih dahulu.');
  if (!nama.trim()) return showMsg(msg, 'Nama pemesan wajib diisi.');
  if (!Number.isInteger(jumlah) || jumlah < 1) return showMsg(msg, 'Jumlah kursi harus bilangan bulat minimal 1.');

  const res = await api.book({ kode, nama, jumlah });
  if (!res.success) return showMsg(msg, res.message);

  showMsg(msg, '');
  ui.lastBooking = res.data;
  toast(`Booking ${res.data.kodeBooking} tersimpan.`);
  $('#bNama').value = '';
  $('#bJumlah').value = 1;
  await renderPesan();
  ui.pesanKode = '';
  showBookingSuccess(res.data);
  renderStats();
  if (ui.mode === 'route') {                         // perbarui sisa kursi pada hasil pencarian
    const r = await api.search(ui.params, { record: false });
    if (r.success) { ui.res = r.data; renderResults(); }
  } else if (ui.mode === 'keyword') {
    await runKeyword(ui.res.kataKunci);
  }
}

function goPesan(kode) {
  ui.pesanKode = kode;
  ui.lastBooking = null;
  goto('pesan');
}

/* ---------- Riwayat ---------- */
async function renderRiwayat() {
  const rows = (await api.bookings()).data;
  $('#tblRiwayat').hidden = rows.length === 0;
  $('#riwayatEmpty').hidden = rows.length !== 0;
  $('#tblRiwayat tbody').innerHTML = rows.map((b) => `
    <tr>
      <td><b>${esc(b.kodeBooking)}</b></td>
      <td>${esc(b.kodeTiket)}</td>
      <td>${esc(b.asal)} → ${esc(b.tujuan)}</td>
      <td>${esc(b.nama)}</td>
      <td class="num">${b.jumlahKursi}</td>
      <td class="num">${rp(b.totalHarga)}</td>
    </tr>`).join('');
}

/* ---------- Log algoritma ---------- */
function cellsHtml(list, sortedUpTo = -1, moved = []) {
  return `<div class="arr">${list.map((c, i) =>
    `<div class="cell ${i <= sortedUpTo ? 'sorted' : ''} ${moved.includes(i) ? 'moved' : ''}">
       <small>[${i}]</small><b>${esc(c.kode)}</b><small>${rp(c.harga)}</small></div>`).join('')}</div>`;
}

function sortingLogHtml(L) {
  if (!L) {
    return `<div class="empty card">${icon('activity', 'big')}<p>Belum ada log sorting. Lakukan pencarian di menu Cari Tiket.</p><a class="btn btn-primary" href="#cari">Ke Cari Tiket</a></div>`;
  }
  const asc = L.urut === 'asc';
  let html = `
    <div class="sum-grid">
      <div class="sum"><small>Urutan</small><b class="sm">${asc ? 'Termurah → termahal' : 'Termahal → termurah'}</b></div>
      <div class="sum"><small>Jumlah data</small><b>${L.n}</b></div>
      <div class="sum"><small>Jumlah perbandingan</small><b>${L.perbandingan}</b></div>
      <div class="sum"><small>Jumlah pertukaran</small><b>${L.pertukaran}</b></div>
    </div>
    <p class="muted">${esc(L.ringkasan)}</p>
    <div class="block-title">Array awal (salinan hasil pencarian)</div>
    ${cellsHtml(L.awal)}`;
  if (L.rounds.length === 0) {
    return html + `<div class="block-title">Hasil akhir</div>${cellsHtml(L.akhir, L.akhir.length - 1)}`;
  }
  html += `
    <div class="block-title">Langkah per putaran</div>
    <div class="legend">
      <span><i style="background:var(--green-50);border-color:#BFE5CC"></i>posisi sudah final</span>
      <span><i style="background:var(--orange-50);border-color:var(--orange)"></i>elemen yang ditukar</span>
    </div>
    <div class="log-tools">
      <button class="btn btn-ghost btn-sm" type="button" data-act="buka-semua">Buka semua putaran</button>
      <button class="btn btn-ghost btn-sm" type="button" data-act="tutup-semua">Tutup semua</button>
    </div>`;
  html += L.rounds.map((r, idx) => {
    const kata = asc ? 'minimum' : 'maksimum';
    const baris = r.komparasi.map((c) => {
      const lawan = asc ? '<' : '>';
      const ket = c.ganti
        ? `<span class="cmp-lt">${rp(c.harga)} ${lawan} ${rp(c.kandidatHarga)}</span> → kandidat baru = indeks ${c.j}`
        : (c.harga === c.kandidatHarga
            ? `<span class="cmp-eq">${rp(c.harga)} = ${rp(c.kandidatHarga)}</span> → sama, kandidat tetap`
            : `<span class="cmp-gt">${rp(c.harga)} ${asc ? '>' : '<'} ${rp(c.kandidatHarga)}</span> → kandidat tetap`);
      return `<tr class="${c.ganti ? 'hit' : ''}">
        <td>[${c.j}] ${esc(c.kode)}</td><td class="num">${rp(c.harga)}</td>
        <td>[${c.kandidatIdx}] ${esc(c.kandidatKode)}</td><td class="decision">${ket}</td></tr>`;
    }).join('');
    const keputusan = r.tukar
      ? `Tukar posisi [${r.posisi}] (${esc(r.kodeI)}) dengan [${r.tukarDenganIdx}] (${esc(r.kodeBest)}).`
      : `Kandidat sudah berada di posisi [${r.posisi}], tidak ada pertukaran.`;
    return `
    <details class="round" ${idx === 0 ? 'open' : ''}>
      <summary>
        <span>Putaran ${r.putaran}</span>
        <span class="muted">posisi [${r.posisi}] · ${r.komparasi.length} perbandingan · ${kata} ${esc(r.kandidat.kode)} (${rp(r.kandidat.harga)})</span>
        <span class="tag ${r.tukar ? 'swap' : 'keep'}">${r.tukar ? 'tukar' : 'tanpa tukar'}</span>
      </summary>
      <div class="round-body">
        <div class="table-wrap"><table class="tbl">
          <thead><tr><th>Dibandingkan</th><th class="num">Harga</th><th>Kandidat saat ini</th><th>Keputusan</th></tr></thead>
          <tbody>${baris}</tbody></table></div>
        <p class="decision" style="margin-top:10px"><b>${keputusan}</b></p>
        <div class="muted" style="font-size:12.5px;margin-top:8px">Keadaan array setelah putaran ${r.putaran}:</div>
        ${cellsHtml(r.keadaan, r.posisi, r.tukar ? [r.posisi, r.tukarDenganIdx] : [])}
      </div>
    </details>`;
  }).join('');
  html += `<div class="block-title">Hasil akhir</div>${cellsHtml(L.akhir, L.akhir.length - 1)}`;
  return html;
}

async function searchingLogHtml(L) {
  if (!L) {
    return `<div class="empty card">${icon('search', 'big')}<p>Belum ada log searching. Lakukan pencarian di menu Cari Tiket.</p><a class="btn btn-primary" href="#cari">Ke Cari Tiket</a></div>`;
  }
  const k = L.kataKunci;
  const baris = L.steps.map((s) => {
    const cls = s.hasil === 'SAMA' ? 'cmp-eq' : (s.hasil === 'LEBIH KECIL' ? 'cmp-lt' : (s.hasil === 'LEBIH BESAR' ? 'cmp-gt' : ''));
    if (s.fase === 'binary') {
      return `<tr class="${s.hasil === 'SAMA' ? 'hit' : ''}"><td>${s.no}</td><td>Binary</td>
        <td class="num">${s.low}</td><td class="num">${s.high}</td><td class="num"><b>${s.mid}</b></td>
        <td>[${s.mid}] ${esc(s.kode)}: ${esc(s.diperiksa)}</td><td class="${cls}">${s.hasil}</td><td class="wrap decision">${esc(s.keputusan)}</td></tr>`;
    }
    return `<tr><td>${s.no}</td><td>Telusur</td>
      <td class="num">-</td><td class="num">-</td><td class="num"><b>${s.idx}</b></td>
      <td>[${s.idx}] ${esc(s.kode)}: ${esc(s.diperiksa)}</td><td class="${cls}">${s.hasil}</td><td class="wrap decision">${esc(s.keputusan)}</td></tr>`;
  }).join('');

  const arr = (await api.tickets()).data;
  const barisArr = arr.map((t, i) => {
    const dalam = L.ditemukan && i >= L.indeksAwal && i <= L.indeksAkhir;
    return `<tr class="${dalam ? 'hit' : ''}"><td>${i}</td><td>${esc(t.kode)}</td><td>${esc(t.asal)} → ${esc(t.tujuan)}</td>
      <td>${tgl(t.tanggal)}</td><td>${esc(t.jam)}</td><td class="num">${rp(t.harga)}</td><td class="num">${t.kursi}</td></tr>`;
  }).join('');

  return `
    <div class="sum-grid">
      <div class="sum"><small>Kata kunci</small><b class="sm">${esc(k.asal)} → ${esc(k.tujuan)}</b></div>
      <div class="sum"><small>Tanggal</small><b class="sm">${k.tanggal ? tgl(k.tanggal) : 'semua'}</b></div>
      <div class="sum"><small>Jumlah perbandingan</small><b>${L.perbandingan}</b></div>
      <div class="sum ${L.ditemukan ? 'good' : 'bad'}"><small>Status</small><b class="sm">${L.ditemukan ? 'Ditemukan' : 'Tidak ditemukan'}</b></div>
      <div class="sum"><small>Tiket pada rute</small><b>${L.jumlahRute}</b></div>
      <div class="sum"><small>Setelah filter tanggal</small><b>${L.jumlahHasil}</b></div>
    </div>
    <p class="muted">${esc(L.ringkasan)}</p>
    <div class="block-title">Langkah pencarian</div>
    <section class="card table-card"><div class="table-wrap"><table class="tbl">
      <thead><tr><th>#</th><th>Fase</th><th class="num">low</th><th class="num">high</th><th class="num">mid / indeks</th><th>Data diperiksa</th><th>Hasil</th><th>Keputusan / rentang berikutnya</th></tr></thead>
      <tbody>${baris}</tbody></table></div></section>
    <details class="round" style="margin-top:18px">
      <summary><span>Isi array utama</span>
        <span class="muted">${arr.length} data · terurut menurut rute: <b>${L.arrayTerurut ? 'ya' : 'TIDAK'}</b></span></summary>
      <div class="round-body"><div class="table-wrap"><table class="tbl">
        <thead><tr><th>Indeks</th><th>Kode</th><th>Rute</th><th>Tanggal</th><th>Jam</th><th class="num">Harga</th><th class="num">Kursi</th></tr></thead>
        <tbody>${barisArr}</tbody></table></div>
        <p class="muted" style="margin-top:8px;font-size:12.5px">Baris berwarna menandai rentang indeks yang cocok dengan rute pencarian.</p>
      </div>
    </details>`;
}

async function renderLogs(name) {
  const logs = (await api.logs()).data;
  const sortTab = name === 'log-sorting';
  $('#tabSort').classList.toggle('on', sortTab);
  $('#tabSearch').classList.toggle('on', !sortTab);
  $('#logSorting').hidden = !sortTab;
  $('#logSearching').hidden = sortTab;
  if (sortTab) $('#logSorting').innerHTML = sortingLogHtml(logs.sorting);
  else $('#logSearching').innerHTML = await searchingLogHtml(logs.searching);
}

/* ---------- Tentang ---------- */
function renderConcepts() {
  const items = [
    { tone: 't-orange', ic: 'layers', judul: 'Array', teks: 'Semua tiket disimpan dalam array berkapasitas tetap. Array utama selalu terurut menurut rute agar Binary Search valid.',
      kode: 'Ticket tickets[MAX_TICKETS];\nint ticketCount = 40;' },
    { tone: 't-blue', ic: 'receipt', judul: 'Struct', teks: 'Satu struct mewakili satu tiket (dan satu booking). Atribut dikelompokkan dalam satu tipe data.',
      kode: 'struct Ticket {\n  std::string kode, operatorBus;\n  std::string asal, tujuan;\n  std::string tanggal, jam;\n  int harga;\n  int kursi;\n};' },
    { tone: 't-green', ic: 'route', judul: 'Pointer', teks: 'Hasil pencarian disimpan sebagai array pointer ke tiket di array utama. Menukar pointer tidak menyalin struct dan tidak mengubah urutan array utama.',
      kode: 'Ticket* hasil[MAX_TICKETS];\nhasil[n++] = &tickets[i];\nint h = hasil[0]->harga;' },
    { tone: 't-purple', ic: 'activity', judul: 'Selection Sort', teks: 'Setiap putaran memilih harga terkecil (atau terbesar) dari bagian yang belum terurut, lalu menukarnya ke posisi depan. Kompleksitas O(n²).',
      kode: 'for (int i = 0; i < n - 1; i++) {\n  int best = i;\n  for (int j = i + 1; j < n; j++)\n    if (hasil[j]->harga < hasil[best]->harga)\n      best = j;\n  if (best != i) {\n    Ticket* tmp = hasil[i];\n    hasil[i] = hasil[best];\n    hasil[best] = tmp;\n  }\n}' },
    { tone: 't-orange', ic: 'search', judul: 'Binary Search', teks: 'Membelah rentang pencarian dua pada tiap langkah. Wajib pada data terurut menurut kunci yang sama (rute). Kompleksitas O(log n).',
      kode: 'int low = 0, high = ticketCount - 1;\nwhile (low <= high) {\n  int mid = (low + high) / 2;\n  int c = compareRoute(&tickets[mid], asal, tujuan);\n  if (c == 0) break;\n  else if (c < 0) low = mid + 1;\n  else high = mid - 1;\n}' },
  ];
  $('#concepts').innerHTML = items.map((c) => `
    <article class="concept">
      <span class="chip ${c.tone}">${icon(c.ic)}${c.judul}</span>
      <h3>${c.judul}</h3>
      <p>${esc(c.teks)}</p>
      <pre><code>${esc(c.kode)}</code></pre>
    </article>`).join('');
}

/* ---------- Event ---------- */
function bindEvents() {
  window.addEventListener('hashchange', route);

  $('#btnMenu').addEventListener('click', openMenu);
  $('#btnCloseMenu').addEventListener('click', closeMenu);
  $('#scrim').addEventListener('click', closeMenu);
  $('#grpLog').addEventListener('click', () => setLogGroup($('#grpLog').getAttribute('aria-expanded') !== 'true'));

  // Pencarian kata kunci di header -> halaman Cari Tiket
  $('#globalSearch').addEventListener('submit', async (e) => {
    e.preventDefault();
    const q = $('#globalQ').value.trim();
    goto('cari');
    if (q) await runKeyword(q);
  });

  $('#quickForm').addEventListener('submit', submitQuick);
  $('#cariForm').addEventListener('submit', (e) => { e.preventDefault(); runSearch(); });
  $('#segUrut').addEventListener('click', (e) => {
    const b = e.target.closest('button[data-urut]');
    if (!b) return;
    $('#cUrut').value = b.dataset.urut;
    runSearch();
  });

  $('#bTiket').addEventListener('change', () => { ui.lastBooking = null; updateBookCalc(); });
  $('#bJumlah').addEventListener('input', updateBookCalc);
  $('#bookForm').addEventListener('submit', submitBooking);

  $('#btnResetLog').addEventListener('click', async () => {
    await api.resetLogs();
    toast('Log dihapus.');
    renderLogs(location.hash.replace('#', '') || 'log-sorting');
  });

  // Carousel
  $('#carPrev').addEventListener('click', () => { setSlide(ui.carIdx - 1); startAuto(); });
  $('#carNext').addEventListener('click', () => { setSlide(ui.carIdx + 1); startAuto(); });
  $('#carDots').addEventListener('click', (e) => {
    const b = e.target.closest('[data-dot]');
    if (b) { setSlide(Number(b.dataset.dot)); startAuto(); }
  });
  $('#carousel').addEventListener('mouseenter', stopAuto);
  $('#carousel').addEventListener('mouseleave', startAuto);
  $('#carousel').addEventListener('focusin', stopAuto);
  $('#carousel').addEventListener('focusout', startAuto);

  // Modal
  $('#modalClose').addEventListener('click', closeModal);
  $('#modal').addEventListener('click', (e) => { if (e.target.id === 'modal') closeModal(); });
  document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') { closeModal(); closeMenu(); }
  });

  // Tombol dengan data-act / data-go (delegasi)
  document.addEventListener('click', async (e) => {
    const go = e.target.closest('[data-go]');
    if (go) { goto(go.dataset.go); return; }

    const b = e.target.closest('[data-act]');
    if (!b) return;
    const act = b.dataset.act;
    if (act === 'detail') openDetail(b.dataset.kode);
    else if (act === 'tutup') closeModal();
    else if (act === 'pesan') { closeModal(); goPesan(b.dataset.kode); }
    else if (act === 'lihat-rute') {
      $('#cAsal').value = b.dataset.asal; $('#cTujuan').value = b.dataset.tujuan; $('#cTanggal').value = '';
      goto('cari');
      await runSearch();
    }
    else if (act === 'buka-semua') $$('#logSorting details.round').forEach((d) => { d.open = true; });
    else if (act === 'tutup-semua') $$('#logSorting details.round').forEach((d) => { d.open = false; });
  });
}

/* ---------- Mulai ---------- */
async function init() {
  bindEvents();
  renderConcepts();
  await fillCities();
  await initCarousel();
  route();
}
init();