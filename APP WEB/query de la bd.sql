-- DROP DATABASE IF EXISTS ProyectoIA;

CREATE DATABASE IF NOT EXISTS ProyectoIA
CHARACTER SET utf8mb4
COLLATE utf8mb4_unicode_ci;

USE ProyectoIA;

-- ==========================================
-- 1. CAPA DE CLIENTES Y NEGOCIO (Normalizada)
-- ==========================================

CREATE TABLE clientes (
    id_cliente INT AUTO_INCREMENT PRIMARY KEY,
    nombre_negocio VARCHAR(100) NOT NULL,
    fecha_registro DATETIME DEFAULT CURRENT_TIMESTAMP,
    -- Columnas para el inicio de sesión de la PWA
    usuario_app VARCHAR(50) UNIQUE,
    contrasena_app VARCHAR(255)
);

CREATE TABLE medios_contacto (
    id_contacto INT AUTO_INCREMENT PRIMARY KEY,
    id_cliente INT NOT NULL,
    tipo_contacto VARCHAR(50) NOT NULL, 
    valor_contacto VARCHAR(150) NOT NULL,
    nombre_encargado VARCHAR(100),
    FOREIGN KEY (id_cliente) REFERENCES clientes(id_cliente) ON DELETE CASCADE
);

CREATE TABLE granjas_sedes (
    id_granja INT AUTO_INCREMENT PRIMARY KEY,
    id_cliente INT NOT NULL,
    nombre_sede VARCHAR(100) NOT NULL,
    latitud DECIMAL(10,8) NOT NULL,  -- Rango: -90.00000000 a 90.00000000
    longitud DECIMAL(11,8) NOT NULL, -- Rango: -180.00000000 a 180.00000000
    FOREIGN KEY (id_cliente) REFERENCES clientes(id_cliente) ON DELETE CASCADE
);

CREATE TABLE clima_sedes (
    id_registro_clima BIGINT AUTO_INCREMENT PRIMARY KEY,
    id_granja INT NOT NULL,
    temp_ambiente DECIMAL(5,2),
    velocidad_viento_kmh DECIMAL(5,2),
    humedad_ambiente DECIMAL(5,2),
    indice_uv DECIMAL(4,2),
    fecha_hora DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (id_granja) REFERENCES granjas_sedes(id_granja) ON DELETE CASCADE
);

CREATE TABLE estanques (
    id_estanque INT AUTO_INCREMENT PRIMARY KEY,
    id_granja INT NOT NULL,
    nombre_estanque VARCHAR(50) NOT NULL,
    volumen_litros DECIMAL(12,2) NOT NULL, 
    FOREIGN KEY (id_granja) REFERENCES granjas_sedes(id_granja) ON DELETE CASCADE
);

-- ==========================================
-- 2. CAPA BIOLÓGICA (Especies y Etapas)
-- ==========================================

CREATE TABLE especies_peces (
    id_especie INT AUTO_INCREMENT PRIMARY KEY,
    nombre_comun VARCHAR(100) NOT NULL,
    nombre_cientifico VARCHAR(100),
    temp_agua_min DECIMAL(5,2) NOT NULL,
    temp_agua_max DECIMAL(5,2) NOT NULL,
    ph_min DECIMAL(4,2) NOT NULL,
    ph_max DECIMAL(4,2) NOT NULL,
    oxigeno_min_mg_l DECIMAL(5,2) NOT NULL,
    turbidez_max_ntu DECIMAL(6,2)
);

CREATE TABLE etapas_crecimiento (
    id_etapa INT AUTO_INCREMENT PRIMARY KEY,
    id_especie INT NOT NULL,
    nombre_etapa VARCHAR(50) NOT NULL, 
    peso_min_gramos DECIMAL(8,2),
    peso_max_gramos DECIMAL(8,2),
    tipo_alimento VARCHAR(100) NOT NULL, 
    tasa_alimentacion_porcentaje DECIMAL(5,2), 
    FOREIGN KEY (id_especie) REFERENCES especies_peces(id_especie) ON DELETE CASCADE
);

CREATE TABLE estanques_especies (
    id_estanque INT NOT NULL,
    id_especie INT NOT NULL,
    id_etapa_actual INT NOT NULL, 
    cantidad_peces INT NOT NULL,
    fecha_siembra DATE,
    PRIMARY KEY (id_estanque, id_especie),
    FOREIGN KEY (id_estanque) REFERENCES estanques(id_estanque) ON DELETE CASCADE,
    FOREIGN KEY (id_especie) REFERENCES especies_peces(id_especie) ON DELETE CASCADE,
    FOREIGN KEY (id_etapa_actual) REFERENCES etapas_crecimiento(id_etapa)
);

-- ==========================================
-- 3. CAPA SANITARIA (Vacunas y Medicinas)
-- ==========================================

CREATE TABLE catalogo_medico (
    id_medicamento INT AUTO_INCREMENT PRIMARY KEY,
    nombre_producto VARCHAR(100) NOT NULL,
    tipo_producto VARCHAR(50) NOT NULL, 
    dosis_mg_por_litro DECIMAL(8,4) NOT NULL 
);

CREATE TABLE protocolos_sanitarios (
    id_protocolo INT AUTO_INCREMENT PRIMARY KEY,
    id_especie INT NOT NULL,
    id_medicamento INT NOT NULL,
    frecuencia_dias INT, 
    notas_aplicacion TEXT,
    FOREIGN KEY (id_especie) REFERENCES especies_peces(id_especie) ON DELETE CASCADE,
    FOREIGN KEY (id_medicamento) REFERENCES catalogo_medico(id_medicamento) ON DELETE CASCADE
);

CREATE TABLE calendario_sanitario (
    id_evento BIGINT AUTO_INCREMENT PRIMARY KEY,
    id_estanque INT NOT NULL,
    id_medicamento INT NOT NULL,
    fecha_programada DATE NOT NULL, 
    estado VARCHAR(20) DEFAULT 'Pendiente', 
    fecha_aplicacion_real DATE, 
    dosis_total_aplicada_mg DECIMAL(10,2), 
    FOREIGN KEY (id_estanque) REFERENCES estanques(id_estanque) ON DELETE CASCADE,
    FOREIGN KEY (id_medicamento) REFERENCES catalogo_medico(id_medicamento) ON DELETE CASCADE
);

-- ==========================================
-- 4. CAPA IOT / HARDWARE (Optimizada para telemetría)
-- ==========================================

CREATE TABLE modelos_dispositivo (
    id_modelo INT AUTO_INCREMENT PRIMARY KEY,
    nombre_modelo VARCHAR(50) NOT NULL, 
    descripcion TEXT,
    tiene_sensor_aire BOOLEAN DEFAULT FALSE,
    tiene_sensor_ph BOOLEAN DEFAULT FALSE,
    tiene_sensor_oxigeno BOOLEAN DEFAULT FALSE,
    tiene_sensor_nivel BOOLEAN DEFAULT FALSE,
    tiene_sensor_turbidez BOOLEAN DEFAULT FALSE
);

CREATE TABLE dispositivos (
    id_dispositivo INT AUTO_INCREMENT PRIMARY KEY,
    id_estanque INT NOT NULL,
    id_modelo INT NOT NULL, 
    mac_address VARCHAR(17) UNIQUE NOT NULL, 
    estado VARCHAR(20) DEFAULT 'Activo',
    fecha_instalacion DATE,
    FOREIGN KEY (id_estanque) REFERENCES estanques(id_estanque) ON DELETE CASCADE,
    FOREIGN KEY (id_modelo) REFERENCES modelos_dispositivo(id_modelo) ON DELETE RESTRICT
);

CREATE TABLE lecturas (
    id_lectura BIGINT AUTO_INCREMENT PRIMARY KEY,
    id_dispositivo INT NOT NULL,
    
    temp_aire DECIMAL(5,2),       
    humedad_aire DECIMAL(5,2),    
    calidad_aire_ppm DECIMAL(8,2),
    
    temp_agua DECIMAL(5,2),       
    ph DECIMAL(4,2),              
    oxigeno_disuelto DECIMAL(5,2),
    nivel_agua_cm DECIMAL(6,2),   
    turbidez_ntu DECIMAL(6,2),    
    
    fecha_hora DATETIME DEFAULT CURRENT_TIMESTAMP,
    
    FOREIGN KEY (id_dispositivo) REFERENCES dispositivos(id_dispositivo) ON DELETE CASCADE
);

-- Índices optimizados
CREATE INDEX idx_dispositivo_fecha ON lecturas(id_dispositivo, fecha_hora);
CREATE INDEX idx_estado_calendario ON calendario_sanitario(estado, fecha_programada); 
CREATE INDEX idx_granja_fecha_clima ON clima_sedes(id_granja, fecha_hora); 

-- ==========================================
-- 5. TRIGGERS (Automatización de PWA)
-- ==========================================

DELIMITER //

CREATE TRIGGER trg_generar_credenciales_cliente
BEFORE INSERT ON clientes
FOR EACH ROW
BEGIN
    -- Generar 'user' seguido de 5 números si viene vacío
    IF NEW.usuario_app IS NULL THEN
        SET NEW.usuario_app = CONCAT('user', FLOOR(RAND() * 90000) + 10000);
    END IF;
    
    -- Generar 6 números aleatorios si viene vacío
    IF NEW.contrasena_app IS NULL THEN
        SET NEW.contrasena_app = CAST(FLOOR(RAND() * 900000) + 100000 AS CHAR);
    END IF;
END;
//

DELIMITER ;