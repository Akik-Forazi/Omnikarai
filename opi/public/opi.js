/* ═══════════════════════════════════════════════════════════
   OPI — opi.js v5.1  |  Fraziym aesthetic × GitHub structure
   Static file compatible — all links use .html paths
   ═══════════════════════════════════════════════════════════ */
'use strict';

const API = 'https://opi-nine.vercel.app/api';

/* ── Path helper ── */
const P = {
  home:      'index.html',
  search:    'search.html',
  pkg:       n => `package.html?name=${encodeURIComponent(n)}`,
  user:      u => `user.html?u=${encodeURIComponent(u)}`,
  login:     'login.html',
  register:  'register.html',
  dashboard: 'dashboard.html',
  settings:  'settings.html',
  help:      'help.html',
};

/* ── Auth ── */
const Auth = {
  getToken(){ return localStorage.getItem('opi_token'); },
  setToken(t){ localStorage.setItem('opi_token', t); },
  clearToken(){ localStorage.removeItem('opi_token'); localStorage.removeItem('opi_user'); },
  getUser(){ try{ return JSON.parse(localStorage.getItem('opi_user')||'null'); }catch{ return null; } },
  setUser(u){ localStorage.setItem('opi_user', JSON.stringify(u)); },
  isLoggedIn(){ return !!this.getToken(); },
  headers(extra={}){
    const h={'Content-Type':'application/json',...extra};
    const t=this.getToken(); if(t) h['Authorization']=`Bearer ${t}`; return h;
  },
  async fetchUser(){
    if(!this.getToken()) return null;
    const r=await fetch(`${API}/auth/me`,{headers:this.headers()});
    if(!r.ok){ this.clearToken(); return null; }
    const u=await r.json().catch(()=>null);
    if(u) this.setUser(u);
    return u;
  },
  async logout(){
    await fetch(`${API}/auth/logout`,{method:'POST',headers:this.headers()}).catch(()=>{});
    this.clearToken();
    window.location.href=P.home;
  }
};

/* ── API helper ── */
async function api(path, opts={}){
  try{
    const r=await fetch(`${API}${path}`,{headers:Auth.headers(opts.headers||{}),...opts});
    let data={};
    const ct=r.headers.get('content-type')||'';
    if(ct.includes('application/json')){
      data=await r.json().catch(()=>({}));
    } else {
      const txt=await r.text().catch(()=>'');
      data={error: txt.length>200 ? 'Server error — please try again.' : txt||'Unknown error'};
    }
    return {ok:r.ok, status:r.status, data};
  }catch(e){
    return {ok:false, status:0, data:{error:'Network error — check your connection.'}};
  }
}

/* ── Utils ── */
const qs   = (s,c=document) => c.querySelector(s);
const qsa  = (s,c=document) => [...c.querySelectorAll(s)];
const esc  = s => String(s||'').replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');
const fmt  = n => { if(!n&&n!==0)return '0'; n=Number(n); if(n>=1e6)return (n/1e6).toFixed(1)+'M'; if(n>=1e3)return (n/1e3).toFixed(1)+'k'; return String(n); };
const elapsed = iso => {
  if(!iso) return '—';
  const d=Math.floor((Date.now()-new Date(iso).getTime())/86400000);
  if(d===0)return 'today'; if(d===1)return 'yesterday';
  if(d<30) return `${d}d ago`;
  if(d<365)return `${Math.floor(d/30)}mo ago`;
  return `${Math.floor(d/365)}y ago`;
};

/* ── Markdown renderer ── */
function md(text){
  if(!text) return '';
  // escape first, then apply markdown rules
  let s = text
    .replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;')
    .replace(/^### (.+)$/gm,'<h3>$1</h3>')
    .replace(/^## (.+)$/gm,'<h2>$1</h2>')
    .replace(/^# (.+)$/gm,'<h1>$1</h1>')
    .replace(/```[\w]*\n?([\s\S]*?)```/g,'<pre><code>$1</code></pre>')
    .replace(/`([^`\n]+)`/g,'<code>$1</code>')
    .replace(/\*\*([^*]+)\*\*/g,'<strong>$1</strong>')
    .replace(/\*([^*\n]+)\*/g,'<em>$1</em>')
    .replace(/\[([^\]]+)\]\(([^)]+)\)/g,'<a href="$2" target="_blank" rel="noopener">$1</a>')
    .replace(/^- (.+)$/gm,'<li>$1</li>')
    .replace(/\n\n+/g,'</p><p>');
  // wrap loose list items
  s = s.replace(/(<li>[\s\S]*?<\/li>\n?)+/g,'<ul>$&</ul>');
  return s;
}

/* ── Package card ── */
function pkgCard(p){
  if(!p) return '';
  const kw=(p.keywords||[]).slice(0,3).map(k=>`<a href="${P.search}?q=${encodeURIComponent(k)}" class="tag" style="font-size:.65rem">${esc(k)}</a>`).join('');
  return `<a class="pkg-card" href="${P.pkg(p.name)}">
    <div class="pkg-card-top">
      <span class="pkg-card-name">${esc(p.name)}</span>
      <span class="pkg-card-ver badge badge-dim">v${esc(p.latest||p.version||'')}</span>
    </div>
    <p class="pkg-card-desc">${esc(p.description||'No description.')}</p>
    <div class="pkg-card-foot">
      <span style="display:flex;align-items:center;gap:.35rem;font-family:var(--mono);font-size:.72rem">
        <svg width="11" height="11" viewBox="0 0 14 14" fill="none"><path d="M7 1v9M3 7l4 4 4-4M2 13h10" stroke="currentColor" stroke-width="1.3" stroke-linecap="round" stroke-linejoin="round"/></svg>
        ${fmt(p.total_downloads||0)}
      </span>
      <div style="display:flex;gap:.3rem;flex-wrap:wrap;justify-content:flex-end">${kw}</div>
    </div>
  </a>`;
}

/* ── NAV ── */
function renderNav(){
  const nav=qs('#nav'); if(!nav) return;
  const user=Auth.getUser();
  const q=new URLSearchParams(location.search).get('q')||'';
  const page=location.pathname.split('/').pop()||'index.html';
  const isActive=name=>page===name||page===name.replace('.html','');

  nav.innerHTML=`
  <div class="nav-inner">
    <a class="nav-logo" href="${P.home}">
      <div class="nav-logo-icon">
        <svg width="16" height="16" viewBox="0 0 20 20" fill="none"><path d="M3 8l7-5 7 5v9H3V8z" stroke="white" stroke-width="1.5" stroke-linejoin="round"/><rect x="7" y="12" width="6" height="5" rx=".5" stroke="white" stroke-width="1.5"/></svg>
      </div>
      <span>OPI</span>
    </a>
    <div class="nav-search">
      <form id="nsf">
        <input id="nq" type="text" placeholder="Search packages…" value="${esc(q)}" autocomplete="off" spellcheck="false">
        <button type="submit" aria-label="Search">
          <svg width="13" height="13" viewBox="0 0 16 16" fill="none"><circle cx="7" cy="7" r="5" stroke="white" stroke-width="1.4"/><path d="M11 11l3 3" stroke="white" stroke-width="1.4" stroke-linecap="round"/></svg>
        </button>
      </form>
    </div>
    <nav class="nav-links" id="nav-links">
      <a href="${P.search}" style="${isActive('search.html')?'color:var(--fg)':''}">Packages</a>
      <a href="${P.help}" style="${isActive('help.html')?'color:var(--fg)':''}">Docs</a>
      <div id="nav-auth"></div>
    </nav>
    <button class="nav-mobile-toggle" id="nav-toggle" aria-expanded="false" aria-label="Menu">Menu</button>
  </div>`;

  const authEl=qs('#nav-auth');
  if(user){
    authEl.innerHTML=`
    <div class="nav-dropdown" id="nav-dd">
      <div class="nav-avatar" id="nav-av" title="@${esc(user.username)}">${esc((user.display_name||user.username||'U')[0].toUpperCase())}</div>
      <div class="nav-dropdown-menu" id="nav-menu">
        <div style="padding:.6rem 1rem .5rem;font-size:.78rem;color:var(--muted);border-bottom:1px solid var(--line)">@${esc(user.username)}</div>
        <a href="${P.dashboard}">Dashboard</a>
        <a href="${P.user(user.username)}">Your profile</a>
        <a href="${P.settings}">Settings</a>
        <hr>
        <a href="#" class="danger" id="nav-logout">Sign out</a>
      </div>
    </div>`;
    qs('#nav-av').onclick=e=>{e.stopPropagation();qs('#nav-menu').classList.toggle('open')};
    qs('#nav-logout').onclick=e=>{e.preventDefault();Auth.logout()};
    document.addEventListener('click',()=>qs('#nav-menu')?.classList.remove('open'));
  } else {
    authEl.innerHTML=`<a href="${P.login}">Sign in</a><a href="${P.register}" class="nav-btn">Register</a>`;
  }

  qs('#nsf').onsubmit=e=>{
    e.preventDefault();
    const v=qs('#nq').value.trim();
    location.href=v?`${P.search}?q=${encodeURIComponent(v)}`:P.search;
  };

  const toggle=qs('#nav-toggle'), links=qs('#nav-links');
  toggle.onclick=()=>{
    const open=links.classList.toggle('open');
    toggle.textContent=open?'Close':'Menu';
    toggle.setAttribute('aria-expanded',String(open));
    document.body.style.overflow=open?'hidden':'';
  };

  const onScroll=()=>nav.classList.toggle('scrolled',scrollY>30);
  addEventListener('scroll',onScroll,{passive:true}); onScroll();
}

/* ── Footer ── */
function renderFooter(){
  const f=qs('#footer'); if(!f) return;
  f.innerHTML=`
  <div class="footer-inner">
    <div class="footer-grid">
      <div class="footer-brand">
        <div style="font-family:var(--mono);font-size:.8rem;font-weight:700;letter-spacing:.15em;text-transform:uppercase;color:var(--fg);display:flex;align-items:center;gap:.5rem">
          <span style="background:linear-gradient(135deg,#8b6bff,#5b35e6);width:22px;height:22px;border-radius:6px;display:flex;align-items:center;justify-content:center">
            <svg width="12" height="12" viewBox="0 0 20 20" fill="none"><path d="M3 8l7-5 7 5v9H3V8z" stroke="white" stroke-width="1.6" stroke-linejoin="round"/><rect x="7" y="12" width="6" height="5" rx=".5" stroke="white" stroke-width="1.6"/></svg>
          </span>OPI
        </div>
        <p>Official package registry for the Omnikarai language. Native x86-64 speed, zero runtime overhead.</p>
        <div class="footer-status"><span class="status-dot"></span>All systems operational</div>
      </div>
      <div class="footer-col">
        <h4>Registry</h4>
        <a href="${P.search}">Browse packages</a>
        <a href="${P.search}?q=math">Math packages</a>
        <a href="${P.search}?q=string">String packages</a>
        <a href="${API}/stats" target="_blank" rel="noopener">API stats</a>
      </div>
      <div class="footer-col">
        <h4>Account</h4>
        <a href="${P.register}">Create account</a>
        <a href="${P.login}">Sign in</a>
        <a href="${P.dashboard}">Dashboard</a>
        <a href="${P.settings}">Settings</a>
      </div>
      <div class="footer-col">
        <h4>Resources</h4>
        <a href="${P.help}">Documentation</a>
        <a href="${P.help}#api">REST API</a>
        <a href="${P.help}#toml">omnikarai.toml</a>
        <a href="${P.help}#tokens">API tokens</a>
      </div>
    </div>
    <div class="footer-bottom">
      <span>© 2026 Fraziym Tech &amp; AI · Omnikarai Package Index</span>
      <span style="display:flex;gap:1rem"><a href="${P.home}" style="color:var(--dim)">OPI</a> · <a href="${P.help}" style="color:var(--dim)">Docs</a> · <a href="${API}/stats" target="_blank" style="color:var(--dim)">API</a></span>
    </div>
  </div>`;
}

/* ── BG ── */
function initBG(){
  if(!qs('.opi-bg')){const d=document.createElement('div');d.className='opi-bg';document.body.prepend(d);}
  if(!qs('.opi-bg-grid')){const d=document.createElement('div');d.className='opi-bg-grid';document.body.prepend(d);}
  if(qs('#opi-particles'))return;
  const canvas=document.createElement('canvas');
  canvas.id='opi-particles';
  canvas.style.cssText='position:fixed;inset:0;z-index:0;pointer-events:none;opacity:.38;';
  document.body.prepend(canvas);
  const ctx=canvas.getContext('2d');
  const resize=()=>{canvas.width=innerWidth;canvas.height=innerHeight};
  resize(); addEventListener('resize',resize,{passive:true});
  const COLS=['rgba(164,139,255,','rgba(90,180,255,','rgba(77,240,200,'];
  const pts=Array.from({length:Math.min(45,Math.floor(innerWidth/28))},()=>({
    x:Math.random()*innerWidth,y:Math.random()*innerHeight,
    r:.4+Math.random()*1.1,vx:(Math.random()-.5)*.14,vy:(Math.random()-.5)*.11,
    a:.1+Math.random()*.35,c:COLS[Math.floor(Math.random()*COLS.length)]
  }));
  (function draw(){
    ctx.clearRect(0,0,canvas.width,canvas.height);
    pts.forEach(p=>{
      p.x+=p.vx;p.y+=p.vy;
      if(p.x<0)p.x=canvas.width;if(p.x>canvas.width)p.x=0;
      if(p.y<0)p.y=canvas.height;if(p.y>canvas.height)p.y=0;
      ctx.beginPath();ctx.arc(p.x,p.y,p.r,0,Math.PI*2);
      ctx.fillStyle=p.c+p.a+')';ctx.fill();
    });
    requestAnimationFrame(draw);
  })();
}

/* ── Reveal ── */
function initReveal(){
  const els=qsa('.reveal');if(!els.length)return;
  if(!('IntersectionObserver'in window)){els.forEach(e=>e.classList.add('visible'));return;}
  const io=new IntersectionObserver(entries=>{
    entries.forEach(e=>{if(e.isIntersecting){e.target.classList.add('visible');io.unobserve(e.target);}});
  },{threshold:.08,rootMargin:'0px 0px -24px 0px'});
  els.forEach(e=>io.observe(e));
}

/* ── Pointer glow ── */
function initPointerLight(){
  document.addEventListener('mousemove',e=>{
    const box=e.target.closest('.card,.pkg-card,.stat-card');
    if(!box)return;
    const r=box.getBoundingClientRect();
    box.style.setProperty('--px',((e.clientX-r.left)/r.width*100).toFixed(1)+'%');
    box.style.setProperty('--py',((e.clientY-r.top)/r.height*100).toFixed(1)+'%');
  },{passive:true});
  const s=document.createElement('style');
  s.textContent=`.card:hover,.pkg-card:hover,.stat-card:hover{background-image:radial-gradient(circle at var(--px,50%) var(--py,50%),rgba(139,107,255,.07) 0%,transparent 60%);}`;
  document.head.appendChild(s);
}

/* ── Copy to clipboard helper ── */
function copyText(text, btn){
  navigator.clipboard.writeText(text).then(()=>{
    const orig=btn.textContent;
    btn.textContent='Copied!';
    btn.style.color='var(--acc5)';
    setTimeout(()=>{btn.textContent=orig;btn.style.color='';},1800);
  }).catch(()=>{});
}

/* ── INIT ── */
document.addEventListener('DOMContentLoaded',async()=>{
  if(Auth.getToken()&&!Auth.getUser()) await Auth.fetchUser();
  initBG();
  renderNav();
  renderFooter();
  initReveal();
  initPointerLight();
  if(typeof pageInit==='function') pageInit();
});
