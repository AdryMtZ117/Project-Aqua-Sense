/* --- CONFIGURACIONES --- */
const OW_API_KEY = '51cfc9826e10be14ad08bbb5b4d2e1bd';
const OW_ICONS = { '01d':'☀️','01n':'🌙','02d':'🌤️','02n':'🌤️','03d':'☁️','03n':'☁️','04d':'☁️','04n':'☁️','09d':'🌧️','09n':'🌧️','10d':'🌦️','10n':'🌦️','11d':'⛈️','11n':'⛈️','13d':'❄️','13n':'❄️','50d':'🌫️','50n':'🌫️' };

let histTemp = []; 
let currentSedeId = null;
let currentSedeName = '';
let masterCatalogos = {};

window.onload = () => {
    obtenerUbicacionYClima();
    verificarSesionUI();
    cargarSucursales();
};

/* --- CLIMA --- */
async function obtenerUbicacionYClima() {
    const elUbi = document.getElementById('ubicacion-actual');
    const btnUbi = document.getElementById('btn-ubicacion');
    if (elUbi) elUbi.textContent = "Ubicando...";
    if (btnUbi) btnUbi.style.display = 'none';

    // Usar última ubicación conocida en caché para una carga instantánea si existe
    const cachedLat = localStorage.getItem('last_clima_lat');
    const cachedLon = localStorage.getItem('last_clima_lon');
    const cachedName = localStorage.getItem('last_clima_name');
    if (cachedLat && cachedLon) {
        fetchClimaFull(cachedLat, cachedLon, cachedName);
    }

    if (navigator.geolocation) {
        navigator.geolocation.getCurrentPosition(
            (p) => {
                fetchClimaFull(p.coords.latitude, p.coords.longitude);
            },
            async (err) => {
                console.warn("GPS denegado o no disponible en navegador, detectando dinámicamente por red:", err);
                if (btnUbi) btnUbi.style.display = 'block';
                await obtenerUbicacionPorIP();
            },
            { enableHighAccuracy: false, timeout: 6000, maximumAge: 60000 }
        );
    } else { 
        await obtenerUbicacionPorIP(); 
    }
}

async function obtenerUbicacionPorIP() {
    // 1. Intentar con freeipapi (alta precisión por IP en México)
    try {
        const res = await fetch('https://freeipapi.com/api/json');
        const data = await res.json();
        if (data && data.latitude && data.longitude) {
            const zona = data.cityName || data.regionName || '';
            await fetchClimaFull(data.latitude, data.longitude, zona);
            return;
        }
    } catch (e) {
        console.warn("Error resolviendo IP con freeipapi:", e);
    }

    // 2. Intentar con ipwho.is (backup seguro con HTTPS y CORS)
    try {
        const res = await fetch('https://ipwho.is/');
        const data = await res.json();
        if (data && data.success && data.latitude && data.longitude) {
            const zona = data.city || data.region || '';
            await fetchClimaFull(data.latitude, data.longitude, zona);
            return;
        }
    } catch (e) {
        console.warn("Error resolviendo IP con ipwho.is:", e);
    }

    // 3. Intentar con BigDataCloud IP client
    try {
        const res = await fetch('https://api.bigdatacloud.net/data/reverse-geocode-client?localityLanguage=es');
        const data = await res.json();
        if (data && data.latitude && data.longitude) {
            const zona = data.locality || data.city || '';
            await fetchClimaFull(data.latitude, data.longitude, zona);
            return;
        }
    } catch (e) {
        console.warn("Error resolviendo IP con BigDataCloud:", e);
    }

    // 4. Último recurso (CDMX) solo si todo falla o está completamente offline
    await fetchClimaFull(19.4326, -99.1332, 'Ciudad de México');
}

function solicitarPermisoUbicacion() {
    alert("Si denegaste el permiso de ubicación anteriormente, debes tocar el ícono de 'candado' o 'información' en la barra de direcciones de tu navegador para permitir el acceso a la ubicación, y luego recargar la página.");
    obtenerUbicacionYClima();
}

async function fetchClimaFull(lat, lon, customName = null) {
    try {
        localStorage.setItem('last_clima_lat', lat);
        localStorage.setItem('last_clima_lon', lon);
    } catch (e) {}
    
    const api1 = `https://api.openweathermap.org/data/2.5/weather?lat=${lat}&lon=${lon}&appid=${OW_API_KEY}&units=metric&lang=es`;
    const api2 = `https://api.openweathermap.org/data/2.5/forecast?lat=${lat}&lon=${lon}&appid=${OW_API_KEY}&units=metric&lang=es`;
    const apiGeo = `https://api.openweathermap.org/geo/1.0/reverse?lat=${lat}&lon=${lon}&limit=5&appid=${OW_API_KEY}`;
    procesarClima(api1, api2, apiGeo, customName, lat, lon);
}

async function fetchClimaByCity(city) {
    const api1 = `https://api.openweathermap.org/data/2.5/weather?q=${city}&appid=${OW_API_KEY}&units=metric&lang=es`;
    const api2 = `https://api.openweathermap.org/data/2.5/forecast?q=${city}&appid=${OW_API_KEY}&units=metric&lang=es`;
    procesarClima(api1, api2, null, city.split(',')[0]);
}

async function procesarClima(u1, u2, geoUrl = null, fallbackName = null, lat = null, lon = null) {
    try {
        const r1 = await fetch(u1); const d1 = await r1.json();
        
        let locName = fallbackName || d1.name;

        // Geocodificación inversa detallada para identificar la alcaldía/municipio exacto si tenemos coordenadas
        if (lat && lon) {
            try {
                const rg = await fetch(`https://api.bigdatacloud.net/data/reverse-geocode-client?latitude=${lat}&longitude=${lon}&localityLanguage=es`);
                const dg = await rg.json();
                if (dg && (dg.locality || dg.city)) {
                    locName = dg.locality || dg.city;
                }
            } catch (e) {
                console.warn("Geo BigDataCloud error:", e);
            }
        }

        // Si aún no tenemos nombre detallado y contamos con la URL de OpenWeather Geo
        if ((!locName || locName === d1.name) && geoUrl) {
            try {
                const rg2 = await fetch(geoUrl); 
                const dg2 = await rg2.json();
                if (dg2 && dg2.length > 0) {
                    locName = dg2[0].local_names?.es || dg2[0].name || locName;
                }
            } catch (e) { console.warn("Geo OpenWeather error:", e); }
        }

        if (locName) {
            try { localStorage.setItem('last_clima_name', locName); } catch (e) {}
        }
        
        document.getElementById('ubicacion-actual').textContent = locName;
        document.getElementById('clima-temp').textContent = Math.round(d1.main.temp);
        
        let desc = d1.weather[0].description;
        let tMax = Math.round(d1.main.temp_max);
        let tMin = Math.round(d1.main.temp_min);
        document.getElementById('clima-desc').textContent = desc;
        document.getElementById('clima-maxmin').textContent = `${tMax}°/${tMin}°`;

        // Llenar datos de sensores con clima real
        let humedad = d1.main.humidity;
        let viento = (d1.wind.speed * 3.6).toFixed(1); // m/s to km/h
        let sensacion = Math.round(d1.main.feels_like);
        let presion = d1.main.pressure;

        document.getElementById('sv-humedad').textContent = humedad;
        document.getElementById('sv-viento').textContent = viento;
        document.getElementById('sv-sensacion').textContent = sensacion;
        document.getElementById('sv-presion').textContent = presion;

        // Anillos SVG para Clima
        setRingProgress('ring-humedad', humedad, 0, 100, true);
        setRingProgress('ring-sensacion', sensacion, -10, 50, true);
        setRingProgress('ring-viento', d1.wind.speed * 3.6, 0, 100, false);
        setRingProgress('ring-presion', presion, 950, 1050, false);

        const r2 = await fetch(u2); const d2 = await r2.json();
        const grid = document.getElementById('pron-grid');
        grid.innerHTML = '';
        
        // Extraer pronóstico agrupando por día para obtener min/max reales diarios
        const diasData = {};
        d2.list.forEach(f => {
            const dateStr = f.dt_txt.split(' ')[0]; // "YYYY-MM-DD"
            if (!diasData[dateStr]) {
                diasData[dateStr] = { dt: f.dt, min: f.main.temp_min, max: f.main.temp_max, icons: [] };
            }
            if (f.main.temp_min < diasData[dateStr].min) diasData[dateStr].min = f.main.temp_min;
            if (f.main.temp_max > diasData[dateStr].max) diasData[dateStr].max = f.main.temp_max;
            // Forzar íconos de día (reemplazar 'n' por 'd') para evitar lunas en el pronóstico
            diasData[dateStr].icons.push(f.weather[0].icon.replace('n', 'd'));
        });

        let diasClaves = Object.keys(diasData);
        // Si el primer día (hoy) está incompleto porque ya pasó parte del día, su min/max será casi igual.
        // Lo saltamos si tiene pocos datos y hay suficientes días por delante.
        if (diasData[diasClaves[0]].icons.length < 5 && diasClaves.length > 5) {
            diasClaves.shift();
        }
        diasClaves = diasClaves.slice(0, 5); // Mostrar 5 días

        diasClaves.forEach(key => {
            const day = diasData[key];
            const dia = new Date(day.dt * 1000).toLocaleDateString('es-MX', {weekday:'short'});
            
            // Seleccionamos el ícono más representativo del día
            const iconCode = day.icons[Math.floor(day.icons.length / 2)];
            
            grid.innerHTML += `
            <div class="pron-row">
                <div class="pron-day">${dia.toUpperCase()}</div>
                <div class="pron-ico">${OW_ICONS[iconCode] || '☀️'}</div>
                <div class="pron-temps">
                    <span class="pron-tmin">${Math.round(day.min)}°</span>
                    <div style="flex:1; height:4px; background:linear-gradient(90deg, #50e3c2, #f5a623); border-radius:2px;"></div>
                    <span class="pron-tmax">${Math.round(day.max)}°</span>
                </div>
            </div>`;
        });

        // Gráfica de Tendencia (Próximas 24 horas - 8 periodos de 3h)
        histTemp = [];
        for(let i=0; i<8; i++) {
            if(d2.list[i]) histTemp.push(d2.list[i].main.temp);
        }
        dibujarGrafica();

    } catch(e) { console.error("Error Clima:", e); }
}

/* --- SENSORES Y GRÁFICOS --- */
function setRingProgress(id, val, min, max, isArc) {
    const el = document.getElementById(id);
    if(!el) return;
    let percentage = (val - min) / (max - min);
    if(percentage > 1) percentage = 1;
    if(percentage < 0) percentage = 0;

    if(isArc) {
        // Longitud total del arco semicircular r=40 es ~125.6
        // Definimos un dasharray de 126
        el.style.strokeDasharray = "126";
        el.style.strokeDashoffset = 126 - (percentage * 126);
    } else {
        // Círculo completo 2 * PI * r = 251.2
        el.style.strokeDasharray = "251.2";
        el.style.strokeDashoffset = 251.2 - (percentage * 251.2);
    }
}

function dibujarGrafica() {
    const canvas = document.getElementById('tend-canvas');
    if(!canvas) return;
    const ctx = canvas.getContext('2d');
    const W = canvas.width = canvas.offsetWidth;
    const H = canvas.height = canvas.offsetHeight;
    const min = Math.min(...histTemp) - 0.5, max = Math.max(...histTemp) + 0.5;
    ctx.clearRect(0,0,W,H);
    ctx.strokeStyle = '#f5e85c'; 
    ctx.lineWidth = 3;
    ctx.lineCap = 'round';
    ctx.lineJoin = 'round';
    ctx.beginPath();
    histTemp.forEach((t, i) => {
        const x = (i / (histTemp.length - 1)) * W;
        const y = H - ((t - min) / (max - min)) * (H - 20) - 10;
        if(i === 0) ctx.moveTo(x,y); else ctx.lineTo(x,y);
    });
    ctx.stroke();

    // Puntos
    histTemp.forEach((t, i) => {
        const x = (i / (histTemp.length - 1)) * W;
        const y = H - ((t - min) / (max - min)) * (H - 20) - 10;
        
        ctx.beginPath();
        if (i === 0) {
            ctx.fillStyle = '#4A90E2';
            ctx.arc(x, y, 6, 0, Math.PI * 2);
            ctx.fill();
            ctx.lineWidth = 2;
            ctx.strokeStyle = '#ffffff';
            ctx.stroke();
        } else {
            ctx.fillStyle = '#ffffff';
            ctx.arc(x, y, 4, 0, Math.PI * 2);
            ctx.fill();
        }
    });
    document.getElementById('t-min').textContent = Math.min(...histTemp).toFixed(1) + '°';
    document.getElementById('t-max').textContent = Math.max(...histTemp).toFixed(1) + '°';
    document.getElementById('t-avg').textContent = (histTemp.reduce((a,b)=>a+b)/histTemp.length).toFixed(1) + '°';
}

/* --- NAVEGACIÓN --- */
function toggleDatos() {
    const viewClima = document.getElementById('view-clima');
    const viewDatos = document.getElementById('view-datos');
    if (viewClima.classList.contains('active')) {
        viewClima.classList.remove('active');
        viewDatos.classList.add('active');
    } else {
        viewDatos.classList.remove('active');
        viewClima.classList.add('active');
        volverALista(); // Por si estaba en detalles de peces
    }
}

/* --- CHAT IA --- */
function abrirChat() {
    document.getElementById('chat-overlay').classList.add('open');
}
function cerrarChat() { 
    document.getElementById('chat-overlay').classList.remove('open'); 
}
function ponerMensaje(texto) {
    document.getElementById('chat-input').value = texto;
}
async function enviarMsg() {
    const input = document.getElementById('chat-input');
    const userText = input.value.trim();
    if(!userText) return;
    
    const msg = document.createElement('div');
    msg.className = 'msg user'; msg.textContent = userText;
    document.getElementById('chat-messages').appendChild(msg);
    input.value = '';
    document.getElementById('chat-messages').scrollTop = document.getElementById('chat-messages').scrollHeight;
    
    // Indicador de "escribiendo"
    const typingMsg = document.createElement('div');
    typingMsg.className = 'msg bot'; 
    typingMsg.textContent = "Pensando...";
    typingMsg.id = 'typing-indicator';
    document.getElementById('chat-messages').appendChild(typingMsg);
    document.getElementById('chat-messages').scrollTop = document.getElementById('chat-messages').scrollHeight;

    // Manejo de la opción de respuesta estricta
    let mensajeAEnviar = userText;
    const chkEstricto = document.getElementById('chat-modo-estricto');
    if (chkEstricto && chkEstricto.checked) {
        mensajeAEnviar += " (Instrucción interna: Responde ÚNICAMENTE con el dato numérico, estado o valor exacto solicitado. No agregues saludos, ni sugerencias adicionales, ni explicaciones extra, solo el dato y su unidad de medida si aplica.)";
    }

    try {
        // REEMPLAZA ESTA URL CON TU WEBHOOK DE N8N (Asegúrate de usar el de Test o Producción correcto)
        const n8nWebhookUrl = 'http://79.143.88.218:5678/webhook/aqua-ia';
        const cliente_id = localStorage.getItem('cliente_id') || '0';

        const res = await fetch(n8nWebhookUrl, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ mensaje: mensajeAEnviar, id_cliente: cliente_id })
        });

        const data = await res.json();
        
        document.getElementById('typing-indicator').remove();
        
        const botMsg = document.createElement('div');
        botMsg.className = 'msg bot'; 
        // n8n debe devolver un JSON estructurado así: { "respuesta": "texto de la IA" }
        botMsg.textContent = data.respuesta || "Lo siento, la IA no devolvió una respuesta válida.";
        document.getElementById('chat-messages').appendChild(botMsg);
        document.getElementById('chat-messages').scrollTop = document.getElementById('chat-messages').scrollHeight;
        
    } catch (e) {
        console.error(e);
        document.getElementById('typing-indicator').remove();
        const errorMsg = document.createElement('div');
        errorMsg.className = 'msg bot'; 
        errorMsg.style.color = '#e74c3c';
        errorMsg.textContent = "Error de red: n8n no está corriendo o la URL del Webhook es incorrecta.";
        document.getElementById('chat-messages').appendChild(errorMsg);
    }
}

/* --- SUCURSALES Y MYSQL --- */
async function cargarSucursales() {
    const lista = document.getElementById('lista-sucursales');
    const cliente_id = localStorage.getItem('cliente_id');
    
    if (!cliente_id) {
        lista.innerHTML = `
            <div style="text-align:center; padding: 30px 10px;">
                <div style="font-size:3rem; margin-bottom:10px;">🔒</div>
                <p style="color:var(--text-dim); margin-bottom:15px;">Inicia sesión con tu cuenta de cliente para sincronizar los datos de tus sensores.</p>
                <button class="btn-datos" style="background:#4A90E2; color:white; padding:10px 20px;" onclick="abrirLogin()">Iniciar Sesión</button>
            </div>
        `;
        return;
    }

    try {
        const res = await fetch(`conn.php?action=sucursales&id_cliente=${cliente_id}`);
        const sucursales = await res.json();
        
        if (sucursales.error) {
            lista.innerHTML = `<p style='text-align:center; padding:20px; color:red;'>Error del servidor: ${sucursales.error}</p>`;
            return;
        }

        if (sucursales.length === 0) {
            lista.innerHTML = "<p style='text-align:center; padding:20px;'>No tienes granjas registradas en tu cuenta.</p>";
            return;
        }

        lista.innerHTML = sucursales.map(s => `
            <div class="sucursal-item" onclick="cargarDatosCascada(${s.id}, '${s.nombre_sucursal}')">
                <div>
                    <div class="sucursal-title">${s.nombre_sucursal}</div>
                    <div class="sucursal-sub">📍 Lat: ${s.latitud} | Lon: ${s.longitud}</div>
                </div>
                <span style="font-size:1.5rem;">🏢</span>
            </div>
        `).join('');
    } catch (e) { 
        console.error("Error en sucursales:", e);
        lista.innerHTML = `<p style='text-align:center; padding:20px; color:red;'>Error conectando al servidor o error de código: ${e.message}</p>`;
    }
}

async function cargarDatosCascada(id, nombre) {
    try {
        currentSedeId = id;
        currentSedeName = nombre;
        
        // Fetch catalogos maestros primero
        const resCat = await fetch(`conn.php?action=get_catalogos_maestros&id_granja=${id}`);
        masterCatalogos = await resCat.json();
        
        document.getElementById('lista-sucursales').style.display = 'none';
        document.getElementById('detalle-sede').style.display = 'block';
        document.getElementById('nombre-sucursal-detalle').innerHTML = `📍 DETALLE SEDE: ${nombre.toUpperCase()}`;
        
        const res = await fetch(`conn.php?action=datos_cascada&id_sucursal=${id}`);
        const data = await res.json();
        const container = document.getElementById('cascada-contenido');
        
        if(data.error) {
            container.innerHTML = `<p style='text-align:center;'>${data.error}</p>`;
            return;
        }

        let html = '';

        // 1. Clima de la Sede
        if (data.clima) {
            html += `
            <div class="cascada-seccion">
                <div class="cascada-titulo">🌤️ Clima de la Sede</div>
                <div class="cascada-grid">
                    <div class="cascada-card"><div class="cascada-item">Temp: <strong>${data.clima.temp_ambiente}°C</strong></div></div>
                    <div class="cascada-card"><div class="cascada-item">Humedad: <strong>${data.clima.humedad_ambiente}%</strong></div></div>
                    <div class="cascada-card"><div class="cascada-item">Viento: <strong>${data.clima.velocidad_viento_kmh} km/h</strong></div></div>
                    <div class="cascada-card"><div class="cascada-item">UV: <strong>${data.clima.indice_uv}</strong></div></div>
                </div>
                <div style="font-size:0.8rem; color:var(--text-dim); margin-top:5px;">Lectura: ${data.clima.fecha_hora}</div>
            </div>`;
        }

        // 2. Protocolos Sanitarios Generales
        html += `<div class="cascada-seccion">
            <div class="cascada-titulo">
                🛡️ Protocolos Sanitarios Generales
                <button class="btn-datos" style="margin-left:auto; font-size:0.8rem; background:rgba(80,227,194,0.3);" onclick="abrirModalProtocolo()">➕ Añadir</button>
            </div>`;
        if (data.protocolos && data.protocolos.length > 0) {
            html += data.protocolos.map(p => `
                <div class="cascada-sub-seccion" style="position:relative;">
                    <div class="cascada-item">Especie: <strong>${p.especie}</strong></div>
                    <div class="cascada-item">Producto: <strong>${p.nombre_producto} (${p.tipo_producto})</strong></div>
                    <div class="cascada-item">Dosis: <strong>${p.dosis_mg_por_litro} mg/L</strong> | Frecuencia: <strong>Cada ${p.frecuencia_dias} días</strong></div>
                    <div class="cascada-item">Notas: <em>${p.notas_aplicacion}</em></div>
                    <div style="position:absolute; top:0; right:0; display:flex; gap:5px;">
                        <button onclick='abrirModalProtocolo(${JSON.stringify(p)})' style="background:none; border:none; color:#f5a623; cursor:pointer;">✏️</button>
                        <button onclick="eliminarProtocolo(${p.id_protocolo})" style="background:none; border:none; color:#e74c3c; cursor:pointer;">🗑️</button>
                    </div>
                </div>
            `).join('');
        } else {
            html += `<div class="cascada-item" style="color:var(--text-dim); font-style:italic;">No hay protocolos sanitarios asignados para las especies de esta granja.</div>`;
        }
        html += `</div>`;

        // 3. Estanques
        if (data.estanques && data.estanques.length > 0) {
            data.estanques.forEach(est => {
                html += `
                <div class="cascada-seccion" style="background: rgba(255,255,255,0.08);">
                    <div class="cascada-titulo" style="color:#50e3c2;">🌊 Estanque: ${est.nombre_estanque}</div>
                    <div class="cascada-item" style="margin-bottom:15px;">Volumen: <strong>${est.volumen_litros} L</strong></div>
                `;

                // Peces
                html += `<div class="cascada-titulo" style="font-size:1rem; color:#f5a623; display:flex; align-items:center;">
                    🐟 Peces y Etapa
                    <button class="btn-datos" style="margin-left:auto; font-size:0.8rem; background:rgba(245,166,35,0.3);" onclick="abrirModalPez(${est.id_estanque})">➕ Añadir</button>
                </div>`;
                if (est.peces && est.peces.length > 0) {
                    est.peces.forEach(pez => {
                        html += `
                        <div class="cascada-sub-seccion" style="position:relative;">
                            <div class="cascada-item">Especie: <strong>${pez.nombre_comun}</strong> <em>(${pez.nombre_cientifico})</em></div>
                            <div class="cascada-item">Cantidad: <strong>${pez.cantidad_peces}</strong> | Siembra: <strong>${pez.fecha_siembra}</strong></div>
                            <div class="cascada-item">Etapa: <strong>${pez.nombre_etapa}</strong> (${pez.peso_min_gramos}g - ${pez.peso_max_gramos}g)</div>
                            <div class="cascada-item">Alimentación: <strong>${pez.tipo_alimento}</strong> (${pez.tasa_alimentacion_porcentaje}%)</div>
                            <div style="position:absolute; top:0; right:0; display:flex; gap:5px;">
                                <button onclick='abrirModalPez(${est.id_estanque}, ${JSON.stringify(pez)})' style="background:none; border:none; color:#f5a623; cursor:pointer;">✏️</button>
                                <button onclick="eliminarPez(${est.id_estanque}, ${pez.id_especie})" style="background:none; border:none; color:#e74c3c; cursor:pointer;">🗑️</button>
                            </div>
                            <details style="margin-top:5px;">
                                <summary style="color:var(--text-dim); cursor:pointer; font-size:0.85rem;">Ver parámetros ideales</summary>
                                <div class="cascada-grid">
                                    <div class="cascada-card"><div class="cascada-item">Temp: ${pez.temp_agua_min}-${pez.temp_agua_max}°C</div></div>
                                    <div class="cascada-card"><div class="cascada-item">pH: ${pez.ph_min}-${pez.ph_max}</div></div>
                                    <div class="cascada-card"><div class="cascada-item">O2 Min: ${pez.oxigeno_min_mg_l} mg/L</div></div>
                                    <div class="cascada-card"><div class="cascada-item">Turbidez Max: ${pez.turbidez_max_ntu} NTU</div></div>
                                </div>
                            </details>
                        </div>`;
                    });
                }

                // Dispositivos
                if (est.dispositivos && est.dispositivos.length > 0) {
                    html += `<div class="cascada-titulo" style="font-size:1rem; color:#6be3ff;">⚙️ Dispositivos IoT</div>`;
                    est.dispositivos.forEach(disp => {
                        html += `
                        <div class="cascada-sub-seccion">
                            <div class="cascada-item">Modelo: <strong>${disp.nombre_modelo}</strong></div>
                            <div class="cascada-item">Estado: <strong style="color:${disp.estado==='Activo'?'#50e3c2':'#f5a623'};">${disp.estado}</strong> | MAC: <strong>${disp.mac_address}</strong></div>
                            <div class="cascada-item" style="font-size:0.8rem; color:var(--text-dim);">Instalado: ${disp.fecha_instalacion}</div>
                            `;
                        
                        if (disp.ultima_lectura) {
                            html += `
                            <div style="margin-top:8px; border-top:1px dashed rgba(255,255,255,0.2); padding-top:8px;">
                                <div class="cascada-item" style="font-size:0.85rem; margin-bottom:5px;"><strong>Última Lectura:</strong></div>
                                <div class="cascada-grid">`;
                            if (disp.tiene_sensor_aire) {
                                html += `<div class="cascada-card"><div class="cascada-item">Aire: ${disp.ultima_lectura.temp_aire}°C, ${disp.ultima_lectura.humedad_aire}%</div></div>`;
                                html += `<div class="cascada-card"><div class="cascada-item">Calidad Aire: ${disp.ultima_lectura.calidad_aire_ppm} ppm</div></div>`;
                            }
                            if (disp.tiene_sensor_ph) html += `<div class="cascada-card"><div class="cascada-item">Agua: ${disp.ultima_lectura.temp_agua}°C</div></div><div class="cascada-card"><div class="cascada-item">pH: ${disp.ultima_lectura.ph}</div></div>`;
                            if (disp.tiene_sensor_oxigeno) html += `<div class="cascada-card"><div class="cascada-item">O2: ${disp.ultima_lectura.oxigeno_disuelto} mg/L</div></div>`;
                            if (disp.tiene_sensor_nivel) html += `<div class="cascada-card"><div class="cascada-item">Nivel: ${disp.ultima_lectura.nivel_agua_cm} cm</div></div>`;
                            if (disp.tiene_sensor_turbidez) html += `<div class="cascada-card"><div class="cascada-item">Turbidez: ${disp.ultima_lectura.turbidez_ntu} NTU</div></div>`;
                            html += `
                                </div>
                                <div style="font-size:0.75rem; color:var(--text-dim); margin-top:5px;">Leído: ${disp.ultima_lectura.fecha_hora}</div>
                            </div>`;
                        } else {
                            html += `<div class="cascada-item" style="color:#f5a623;">No hay lecturas recientes.</div>`;
                        }
                        
                        html += `</div>`;
                    });
                }

                // Calendario Sanitario
                html += `<div class="cascada-titulo" style="font-size:1rem; color:#f56c6c; margin-top:15px; display:flex; align-items:center;">
                    📅 Calendario Sanitario
                    <button class="btn-datos" style="margin-left:auto; font-size:0.8rem; background:rgba(245,108,108,0.3);" onclick="abrirModalCalendario(${est.id_estanque})">➕ Añadir</button>
                </div>`;
                if (est.calendario && est.calendario.length > 0) {
                    est.calendario.forEach(cal => {
                        let colorEstado = cal.estado === 'Completado' ? '#50e3c2' : (cal.estado === 'Pendiente' ? '#f5a623' : '#e74c3c');
                        html += `
                        <div class="cascada-sub-seccion" style="position:relative;">
                            <div class="cascada-item">Fecha Programada: <strong>${cal.fecha_programada}</strong></div>
                            <div class="cascada-item">Producto: <strong>${cal.nombre_producto} (${cal.tipo_producto})</strong></div>
                            <div class="cascada-item">Dosis/L: <strong>${cal.dosis_mg_por_litro} mg/L</strong></div>
                            <div class="cascada-item">Estado: <strong style="color:${colorEstado};">${cal.estado}</strong></div>
                            ${cal.fecha_aplicacion_real ? `<div class="cascada-item">Aplicado: <strong>${cal.fecha_aplicacion_real}</strong> (Dosis Total: ${cal.dosis_total_aplicada_mg}mg)</div>` : ''}
                            <div style="position:absolute; top:0; right:0; display:flex; gap:5px;">
                                <button onclick='abrirModalCalendario(${est.id_estanque}, ${JSON.stringify(cal)})' style="background:none; border:none; color:#f5a623; cursor:pointer;">✏️</button>
                                <button onclick="eliminarCalendario(${cal.id_evento})" style="background:none; border:none; color:#e74c3c; cursor:pointer;">🗑️</button>
                            </div>
                        </div>`;
                    });
                } else {
                    html += `<div class="cascada-item" style="color:var(--text-dim); font-style:italic;">No hay eventos en el calendario sanitario para este estanque.</div>`;
                }

                html += `</div>`; // fin estanque
            });
        }
        
        // 4. Catálogo Médico General
        html += `<div class="cascada-seccion">
            <div class="cascada-titulo" style="display:flex; align-items:center;">
                💊 Catálogo Médico Disponible
                <button class="btn-datos" style="margin-left:auto; font-size:0.8rem; background:rgba(74,144,226,0.5);" onclick="abrirModalCatalogo()">➕ Añadir</button>
            </div>`;
        if (data.catalogo && data.catalogo.length > 0) {
            html += `<div class="cascada-grid">`;
            data.catalogo.forEach(cat => {
                html += `
                <div class="cascada-card" style="position:relative;">
                    <div class="cascada-item" style="margin-bottom:4px;"><strong>${cat.nombre_producto}</strong></div>
                    <div class="cascada-item" style="font-size:0.8rem;">Tipo: ${cat.tipo_producto}</div>
                    <div class="cascada-item" style="font-size:0.8rem;">Dosis base: ${cat.dosis_mg_por_litro} mg/L</div>
                    <div style="position:absolute; top:5px; right:5px; display:flex; gap:5px;">
                        <button onclick='abrirModalCatalogo(${JSON.stringify(cat)})' style="background:none; border:none; color:#f5a623; cursor:pointer;">✏️</button>
                        <button onclick="eliminarCatalogo(${cat.id_medicamento})" style="background:none; border:none; color:#e74c3c; cursor:pointer;">🗑️</button>
                    </div>
                </div>`;
            });
            html += `</div>`;
        } else {
            html += `<div class="cascada-item" style="color:var(--text-dim); font-style:italic;">El catálogo médico está vacío.</div>`;
        }
        html += `</div>`;

        if (html === '') {
            html = `<p style='text-align:center;'>No hay datos registrados en esta sede.</p>`;
        }

        container.innerHTML = html;
        
    } catch (e) { 
        console.error("Error cargando cascada", e); 
        document.getElementById('cascada-contenido').innerHTML = "<p style='text-align:center; color:red;'>Error cargando detalles.</p>";
    }
}

function volverALista() {
    document.getElementById('detalle-sede').style.display = 'none';
    document.getElementById('lista-sucursales').style.display = 'block';
}

/* --- AUTENTICACIÓN --- */
function verificarSesionUI() {
    const btn = document.getElementById('btn-perfil');
    if (localStorage.getItem('cliente_id')) {
        btn.style.background = '#4A90E2';
        btn.style.color = '#fff';
    } else {
        btn.style.background = 'rgba(255,255,255,0.2)';
        btn.style.color = 'var(--text-main)';
    }
}

function abrirLogin() {
    document.getElementById('login-error').style.display = 'none';
    if (localStorage.getItem('cliente_id')) {
        document.getElementById('login-form').style.display = 'none';
        document.getElementById('login-info').style.display = 'flex';
        document.getElementById('login-client-name').textContent = localStorage.getItem('cliente_nombre');
    } else {
        document.getElementById('login-form').style.display = 'flex';
        document.getElementById('login-info').style.display = 'none';
    }
    document.getElementById('login-overlay').classList.add('open');
}

function cerrarLogin() {
    document.getElementById('login-overlay').classList.remove('open');
}

async function iniciarSesion() {
    const user = document.getElementById('login-user').value.trim();
    const pass = document.getElementById('login-pass').value.trim();
    const errorMsg = document.getElementById('login-error');
    
    if(!user || !pass) {
        errorMsg.textContent = "Ingresa usuario y contraseña.";
        errorMsg.style.display = 'block';
        return;
    }
    
    const formData = new FormData();
    formData.append('usuario', user);
    formData.append('password', pass);
    
    try {
        const res = await fetch('conn.php?action=login', { method: 'POST', body: formData });
        const data = await res.json();
        
        if (data.status === 'success') {
            localStorage.setItem('cliente_id', data.data.id_cliente);
            localStorage.setItem('cliente_nombre', data.data.nombre_negocio);
            verificarSesionUI();
            cargarSucursales();
            cerrarLogin();
            document.getElementById('login-user').value = '';
            document.getElementById('login-pass').value = '';
        } else {
            errorMsg.textContent = data.message || "Credenciales incorrectas.";
            errorMsg.style.display = 'block';
        }
    } catch (e) {
        errorMsg.textContent = "Error de conexión.";
        errorMsg.style.display = 'block';
    }
}

function cerrarSesion() {
    localStorage.removeItem('cliente_id');
    localStorage.removeItem('cliente_nombre');
    verificarSesionUI();
    cargarSucursales();
    cerrarLogin();
}

/* --- CRUD LOGIC --- */
function cerrarModal(id) {
    document.getElementById(id).classList.remove('open');
}

// PECES
function abrirModalPez(id_estanque, pez = null) {
    const sEstanque = document.getElementById('crud-pez-estanque');
    const sEspecie = document.getElementById('crud-pez-especie');
    const sEtapa = document.getElementById('crud-pez-etapa');
    
    sEstanque.innerHTML = masterCatalogos.estanques.map(e => `<option value="${e.id_estanque}">${e.nombre_estanque}</option>`).join('');
    sEspecie.innerHTML = masterCatalogos.especies.map(e => `<option value="${e.id_especie}">${e.nombre_comun}</option>`).join('');
    sEtapa.innerHTML = masterCatalogos.etapas.map(e => `<option value="${e.id_etapa}" data-esp="${e.id_especie}">${e.nombre_etapa}</option>`).join('');
    
    document.getElementById('crud-pez-estanque').value = id_estanque;
    
    if(pez) {
        document.getElementById('titulo-modal-pez').textContent = "✏️ Editar Pez";
        document.getElementById('crud-pez-especie').value = pez.id_especie;
        document.getElementById('crud-pez-etapa').value = pez.id_etapa;
        document.getElementById('crud-pez-cantidad').value = pez.cantidad_peces;
        document.getElementById('crud-pez-fecha').value = pez.fecha_siembra;
        document.getElementById('crud-pez-estanque-old').value = id_estanque;
        document.getElementById('crud-pez-especie-old').value = pez.id_especie;
        document.getElementById('crud-pez-estanque').disabled = true;
        document.getElementById('crud-pez-especie').disabled = true;
    } else {
        document.getElementById('titulo-modal-pez').textContent = "🐟 Añadir Pez";
        document.getElementById('crud-pez-cantidad').value = '';
        document.getElementById('crud-pez-fecha').value = new Date().toISOString().split('T')[0];
        document.getElementById('crud-pez-estanque-old').value = '';
        document.getElementById('crud-pez-especie-old').value = '';
        document.getElementById('crud-pez-estanque').disabled = false;
        document.getElementById('crud-pez-especie').disabled = false;
    }
    document.getElementById('modal-pez').classList.add('open');
}

async function guardarPez() {
    const isNew = !document.getElementById('crud-pez-estanque-old').value;
    const payload = {
        is_new: isNew,
        id_estanque: document.getElementById('crud-pez-estanque').value,
        id_especie: document.getElementById('crud-pez-especie').value,
        id_etapa: document.getElementById('crud-pez-etapa').value,
        cantidad: document.getElementById('crud-pez-cantidad').value,
        fecha_siembra: document.getElementById('crud-pez-fecha').value
    };
    if(!payload.id_estanque || !payload.id_especie || !payload.id_etapa || !payload.cantidad || !payload.fecha_siembra) return alert('Completa los campos');
    
    await fetch('conn.php?action=crud_peces', { method: 'POST', body: JSON.stringify(payload) });
    cerrarModal('modal-pez');
    cargarDatosCascada(currentSedeId, currentSedeName);
}

async function eliminarPez(id_estanque, id_especie) {
    if(!confirm('¿Eliminar este registro de peces?')) return;
    await fetch('conn.php?action=crud_peces', { method: 'POST', body: JSON.stringify({delete:true, id_estanque, id_especie}) });
    cargarDatosCascada(currentSedeId, currentSedeName);
}

// CATALOGO MEDICO
function abrirModalCatalogo(cat = null) {
    if(cat) {
        document.getElementById('titulo-modal-catalogo').textContent = "✏️ Editar Medicamento";
        document.getElementById('crud-cat-id').value = cat.id_medicamento;
        document.getElementById('crud-cat-nombre').value = cat.nombre_producto;
        document.getElementById('crud-cat-tipo').value = cat.tipo_producto;
        document.getElementById('crud-cat-dosis').value = cat.dosis_mg_por_litro;
    } else {
        document.getElementById('titulo-modal-catalogo').textContent = "💊 Añadir Medicamento";
        document.getElementById('crud-cat-id').value = '';
        document.getElementById('crud-cat-nombre').value = '';
        document.getElementById('crud-cat-tipo').value = '';
        document.getElementById('crud-cat-dosis').value = '';
    }
    document.getElementById('modal-catalogo').classList.add('open');
}

async function guardarCatalogo() {
    const payload = {
        id: document.getElementById('crud-cat-id').value,
        nombre: document.getElementById('crud-cat-nombre').value,
        tipo: document.getElementById('crud-cat-tipo').value,
        dosis: document.getElementById('crud-cat-dosis').value
    };
    if(!payload.nombre || !payload.tipo || !payload.dosis) return alert('Completa los campos');
    await fetch('conn.php?action=crud_catalogo', { method: 'POST', body: JSON.stringify(payload) });
    cerrarModal('modal-catalogo');
    cargarDatosCascada(currentSedeId, currentSedeName);
}

async function eliminarCatalogo(id) {
    if(!confirm('¿Eliminar medicamento del catálogo?')) return;
    await fetch('conn.php?action=crud_catalogo', { method: 'POST', body: JSON.stringify({delete:true, id}) });
    cargarDatosCascada(currentSedeId, currentSedeName);
}

// PROTOCOLOS SANITARIOS
function abrirModalProtocolo(prot = null) {
    const sEspecie = document.getElementById('crud-prot-especie');
    const sMed = document.getElementById('crud-prot-medicamento');
    sEspecie.innerHTML = masterCatalogos.especies.map(e => `<option value="${e.id_especie}">${e.nombre_comun}</option>`).join('');
    sMed.innerHTML = masterCatalogos.medicamentos.map(m => `<option value="${m.id_medicamento}">${m.nombre_producto} (${m.tipo_producto})</option>`).join('');
    
    if(prot) {
        document.getElementById('titulo-modal-protocolo').textContent = "✏️ Editar Protocolo";
        document.getElementById('crud-prot-id').value = prot.id_protocolo;
        document.getElementById('crud-prot-especie').value = prot.id_especie;
        document.getElementById('crud-prot-medicamento').value = prot.id_medicamento;
        document.getElementById('crud-prot-frecuencia').value = prot.frecuencia_dias;
        document.getElementById('crud-prot-notas').value = prot.notas_aplicacion;
    } else {
        document.getElementById('titulo-modal-protocolo').textContent = "🛡️ Añadir Protocolo";
        document.getElementById('crud-prot-id').value = '';
        document.getElementById('crud-prot-frecuencia').value = '';
        document.getElementById('crud-prot-notas').value = '';
    }
    document.getElementById('modal-protocolo').classList.add('open');
}

async function guardarProtocolo() {
    const payload = {
        id: document.getElementById('crud-prot-id').value,
        id_especie: document.getElementById('crud-prot-especie').value,
        id_medicamento: document.getElementById('crud-prot-medicamento').value,
        frecuencia: document.getElementById('crud-prot-frecuencia').value,
        notas: document.getElementById('crud-prot-notas').value
    };
    if(!payload.id_especie || !payload.id_medicamento || !payload.frecuencia) return alert('Completa los campos obligatorios');
    await fetch('conn.php?action=crud_protocolo', { method: 'POST', body: JSON.stringify(payload) });
    cerrarModal('modal-protocolo');
    cargarDatosCascada(currentSedeId, currentSedeName);
}

async function eliminarProtocolo(id) {
    if(!confirm('¿Eliminar este protocolo?')) return;
    await fetch('conn.php?action=crud_protocolo', { method: 'POST', body: JSON.stringify({delete:true, id}) });
    cargarDatosCascada(currentSedeId, currentSedeName);
}

// CALENDARIO SANITARIO
function abrirModalCalendario(id_estanque, cal = null) {
    const sEst = document.getElementById('crud-cal-estanque');
    const sMed = document.getElementById('crud-cal-medicamento');
    sEst.innerHTML = masterCatalogos.estanques.map(e => `<option value="${e.id_estanque}">${e.nombre_estanque}</option>`).join('');
    sMed.innerHTML = masterCatalogos.medicamentos.map(m => `<option value="${m.id_medicamento}">${m.nombre_producto} (${m.tipo_producto})</option>`).join('');
    
    document.getElementById('crud-cal-estanque').value = id_estanque;
    
    if(cal) {
        document.getElementById('titulo-modal-calendario').textContent = "✏️ Editar Evento";
        document.getElementById('crud-cal-id').value = cal.id_evento;
        document.getElementById('crud-cal-medicamento').value = cal.id_medicamento;
        document.getElementById('crud-cal-fecha-prog').value = cal.fecha_programada;
        document.getElementById('crud-cal-estado').value = cal.estado;
        document.getElementById('crud-cal-fecha-real').value = cal.fecha_aplicacion_real || '';
        document.getElementById('crud-cal-dosis').value = cal.dosis_total_aplicada_mg || '';
    } else {
        document.getElementById('titulo-modal-calendario').textContent = "📅 Añadir Evento";
        document.getElementById('crud-cal-id').value = '';
        document.getElementById('crud-cal-fecha-prog').value = new Date().toISOString().split('T')[0];
        document.getElementById('crud-cal-estado').value = 'Pendiente';
        document.getElementById('crud-cal-fecha-real').value = '';
        document.getElementById('crud-cal-dosis').value = '';
    }
    document.getElementById('modal-calendario').classList.add('open');
}

async function guardarCalendario() {
    const payload = {
        id: document.getElementById('crud-cal-id').value,
        id_estanque: document.getElementById('crud-cal-estanque').value,
        id_medicamento: document.getElementById('crud-cal-medicamento').value,
        fecha_programada: document.getElementById('crud-cal-fecha-prog').value,
        estado: document.getElementById('crud-cal-estado').value,
        fecha_real: document.getElementById('crud-cal-fecha-real').value,
        dosis: document.getElementById('crud-cal-dosis').value
    };
    if(!payload.id_estanque || !payload.id_medicamento || !payload.fecha_programada) return alert('Completa los campos obligatorios');
    await fetch('conn.php?action=crud_calendario', { method: 'POST', body: JSON.stringify(payload) });
    cerrarModal('modal-calendario');
    cargarDatosCascada(currentSedeId, currentSedeName);
}

async function eliminarCalendario(id) {
    if(!confirm('¿Eliminar este evento del calendario?')) return;
    await fetch('conn.php?action=crud_calendario', { method: 'POST', body: JSON.stringify({delete:true, id}) });
    cargarDatosCascada(currentSedeId, currentSedeName);
}