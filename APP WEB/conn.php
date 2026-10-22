<?php
error_reporting(E_ALL);
ini_set('display_errors', 0); // We will handle errors in JSON

header('Content-Type: application/json');
header("Access-Control-Allow-Origin: *");

$db_host = "db_ecosense";
$db_user = "ecosense_user";
$db_pass = "user123";
$db_name = "ecosense_db";

// Soporte para configuración local segura (protegida por .gitignore)
if (file_exists(__DIR__ . '/config.local.php')) {
    include __DIR__ . '/config.local.php';
}

$conn = new mysqli($db_host, $db_user, $db_pass, $db_name);

if ($conn->connect_error) {
    header('Content-Type: application/json');
    echo json_encode(["status" => "error", "message" => "Error de conexión: " . $conn->connect_error]);
    exit;
}

// Para capturar cualquier excepción no manejada y regresarla como JSON
set_exception_handler(function($e) {
    echo json_encode(["error" => "PHP Exception: " . $e->getMessage()]);
    exit;
});

$action = $_GET['action'] ?? '';


if ($action == 'login' && isset($_POST['usuario']) && isset($_POST['password'])) {
    $user = $_POST['usuario'];
    $pass = $_POST['password'];
    $stmt = $conn->prepare("SELECT id_cliente, nombre_negocio FROM clientes WHERE usuario_app = ? AND contrasena_app = ?");
    $stmt->bind_param("ss", $user, $pass);
    $stmt->execute();
    $res = $stmt->get_result();
    if ($row = $res->fetch_assoc()) {
        echo json_encode(["status" => "success", "data" => $row]);
    } else {
        echo json_encode(["status" => "error", "message" => "Credenciales incorrectas"]);
    }
}
elseif ($action == 'sucursales' && isset($_GET['id_cliente'])) {
    $id_cliente = intval($_GET['id_cliente']);
    $stmt = $conn->prepare("SELECT id_granja as id, nombre_sede as nombre_sucursal, latitud, longitud FROM granjas_sedes WHERE id_cliente = ?");
    $stmt->bind_param("i", $id_cliente);
    $stmt->execute();
    echo json_encode($stmt->get_result()->fetch_all(MYSQLI_ASSOC));
} 

// DATOS CASCADA (SEDE COMPLETA)
elseif ($action == 'datos_cascada' && isset($_GET['id_sucursal'])) {
    $id_granja = intval($_GET['id_sucursal']);
    
    // 1. Info Granja
    $stmt = $conn->prepare("SELECT nombre_sede FROM granjas_sedes WHERE id_granja = ?");
    $stmt->bind_param("i", $id_granja);
    $stmt->execute();
    $res_granja = $stmt->get_result()->fetch_assoc();
    if (!$res_granja) {
        echo json_encode(["error" => "Granja no encontrada"]);
        exit;
    }
    
    $response = [
        "nombre_sede" => $res_granja['nombre_sede'],
        "clima" => null,
        "protocolos" => [],
        "estanques" => []
    ];
    
    // 2. Clima
    $stmt = $conn->prepare("SELECT temp_ambiente, velocidad_viento_kmh, humedad_ambiente, indice_uv, fecha_hora FROM clima_sedes WHERE id_granja = ? ORDER BY fecha_hora DESC LIMIT 1");
    $stmt->bind_param("i", $id_granja);
    $stmt->execute();
    $response['clima'] = $stmt->get_result()->fetch_assoc();
    
    // 3. Estanques y sus relaciones
    $stmt = $conn->prepare("SELECT id_estanque, nombre_estanque, volumen_litros FROM estanques WHERE id_granja = ?");
    $stmt->bind_param("i", $id_granja);
    $stmt->execute();
    $estanques_res = $stmt->get_result();
    
    $especies_ids = []; // para protocolos
    
    while ($est = $estanques_res->fetch_assoc()) {
        $estanque_data = [
            "nombre_estanque" => $est['nombre_estanque'],
            "volumen_litros" => $est['volumen_litros'],
            "peces" => [],
            "dispositivos" => [],
            "calendario" => []
        ];
        
        $id_est = $est['id_estanque'];
        // KEEP id_estanque
        
        // Peces
        $stmt_peces = $conn->prepare("
            SELECT e.nombre_comun, e.nombre_cientifico, e.temp_agua_min, e.temp_agua_max, e.ph_min, e.ph_max, e.oxigeno_min_mg_l, e.turbidez_max_ntu,
                   ee.cantidad_peces, ee.fecha_siembra,
                   et.nombre_etapa, et.peso_min_gramos, et.peso_max_gramos, et.tipo_alimento, et.tasa_alimentacion_porcentaje,
                   e.id_especie, et.id_etapa
            FROM estanques_especies ee
            JOIN especies_peces e ON ee.id_especie = e.id_especie
            JOIN etapas_crecimiento et ON ee.id_etapa_actual = et.id_etapa
            WHERE ee.id_estanque = ?
        ");
        $stmt_peces->bind_param("i", $id_est);
        $stmt_peces->execute();
        $peces_res = $stmt_peces->get_result();
        while ($pez = $peces_res->fetch_assoc()) {
            $especies_ids[$pez['id_especie']] = true;
            // KEEP id_especie and id_etapa
            $estanque_data['peces'][] = $pez;
        }
        
        // Dispositivos y Lecturas
        $stmt_disp = $conn->prepare("
            SELECT d.id_dispositivo, d.mac_address, d.estado, d.fecha_instalacion,
                   m.nombre_modelo, m.descripcion, m.tiene_sensor_aire, m.tiene_sensor_ph, m.tiene_sensor_oxigeno, m.tiene_sensor_nivel, m.tiene_sensor_turbidez
            FROM dispositivos d
            JOIN modelos_dispositivo m ON d.id_modelo = m.id_modelo
            WHERE d.id_estanque = ?
        ");
        $stmt_disp->bind_param("i", $id_est);
        $stmt_disp->execute();
        $disp_res = $stmt_disp->get_result();
        while ($disp = $disp_res->fetch_assoc()) {
            $id_disp = $disp['id_dispositivo'];
            // KEEP id_dispositivo
            
            // Ultima lectura
            $stmt_lect = $conn->prepare("
                SELECT temp_aire, humedad_aire, calidad_aire_ppm, temp_agua, ph, oxigeno_disuelto, nivel_agua_cm, turbidez_ntu, fecha_hora
                FROM lecturas
                WHERE id_dispositivo = ?
                ORDER BY fecha_hora DESC LIMIT 1
            ");
            $stmt_lect->bind_param("i", $id_disp);
            $stmt_lect->execute();
            $disp['ultima_lectura'] = $stmt_lect->get_result()->fetch_assoc();
            
            $estanque_data['dispositivos'][] = $disp;
        }
        
        // Calendario Sanitario
        $stmt_cal = $conn->prepare("
            SELECT cs.id_evento, cs.id_medicamento, cs.fecha_programada, cs.estado, cs.fecha_aplicacion_real, cs.dosis_total_aplicada_mg,
                   cm.nombre_producto, cm.tipo_producto, cm.dosis_mg_por_litro
            FROM calendario_sanitario cs
            JOIN catalogo_medico cm ON cs.id_medicamento = cm.id_medicamento
            WHERE cs.id_estanque = ?
        ");
        $stmt_cal->bind_param("i", $id_est);
        $stmt_cal->execute();
        $cal_res = $stmt_cal->get_result();
        while ($cal = $cal_res->fetch_assoc()) {
            $estanque_data['calendario'][] = $cal;
        }
        
        $response['estanques'][] = $estanque_data;
    }
    
    // Protocolos sanitarios (basado en especies en la granja)
    if (!empty($especies_ids)) {
        $ids = implode(',', array_keys($especies_ids));
        $stmt_prot = $conn->query("
            SELECT p.id_protocolo, p.id_especie, p.id_medicamento, p.frecuencia_dias, p.notas_aplicacion,
                   e.nombre_comun as especie,
                   cm.nombre_producto, cm.tipo_producto, cm.dosis_mg_por_litro
            FROM protocolos_sanitarios p
            JOIN especies_peces e ON p.id_especie = e.id_especie
            JOIN catalogo_medico cm ON p.id_medicamento = cm.id_medicamento
            WHERE p.id_especie IN ($ids)
        ");
        if ($stmt_prot) {
            while ($prot = $stmt_prot->fetch_assoc()) {
                $response['protocolos'][] = $prot;
            }
        }
    }
    
    // Catalogo Medico Completo
    $response['catalogo'] = [];
    $stmt_cat = $conn->query("SELECT id_medicamento, nombre_producto, tipo_producto, dosis_mg_por_litro FROM catalogo_medico");
    if ($stmt_cat) {
        while ($cat = $stmt_cat->fetch_assoc()) {
            $response['catalogo'][] = $cat;
        }
    }
    
    echo json_encode($response);
}

// GET CATALOGOS MAESTROS
elseif ($action == 'get_catalogos_maestros' && isset($_GET['id_granja'])) {
    $id_granja = intval($_GET['id_granja']);
    $data = ["especies" => [], "etapas" => [], "estanques" => [], "medicamentos" => []];
    
    $res = $conn->query("SELECT id_especie, nombre_comun FROM especies_peces");
    while($row = $res->fetch_assoc()) $data['especies'][] = $row;
    
    $res = $conn->query("SELECT id_etapa, id_especie, nombre_etapa FROM etapas_crecimiento");
    while($row = $res->fetch_assoc()) $data['etapas'][] = $row;
    
    $stmt = $conn->prepare("SELECT id_estanque, nombre_estanque FROM estanques WHERE id_granja = ?");
    $stmt->bind_param("i", $id_granja);
    $stmt->execute();
    $res = $stmt->get_result();
    while($row = $res->fetch_assoc()) $data['estanques'][] = $row;
    
    $res = $conn->query("SELECT id_medicamento, nombre_producto, tipo_producto FROM catalogo_medico");
    while($row = $res->fetch_assoc()) $data['medicamentos'][] = $row;
    
    echo json_encode($data);
}

// CRUD PECES (ESTANQUES_ESPECIES)
elseif ($action == 'crud_peces') {
    $input = json_decode(file_get_contents('php://input'), true);
    if(isset($input['delete'])) {
        $stmt = $conn->prepare("DELETE FROM estanques_especies WHERE id_estanque=? AND id_especie=?");
        $stmt->bind_param("ii", $input['id_estanque'], $input['id_especie']);
        echo json_encode(["success" => $stmt->execute()]);
    } else {
        $id_estanque = intval($input['id_estanque']);
        $id_especie = intval($input['id_especie']);
        $id_etapa = intval($input['id_etapa']);
        $cantidad = intval($input['cantidad']);
        $fecha = $input['fecha_siembra'];
        
        if(isset($input['is_new']) && $input['is_new']) {
            $stmt = $conn->prepare("INSERT INTO estanques_especies (id_estanque, id_especie, id_etapa_actual, cantidad_peces, fecha_siembra) VALUES (?, ?, ?, ?, ?)");
            $stmt->bind_param("iiiis", $id_estanque, $id_especie, $id_etapa, $cantidad, $fecha);
        } else {
            $stmt = $conn->prepare("UPDATE estanques_especies SET id_etapa_actual=?, cantidad_peces=?, fecha_siembra=? WHERE id_estanque=? AND id_especie=?");
            $stmt->bind_param("iisii", $id_etapa, $cantidad, $fecha, $id_estanque, $id_especie);
        }
        echo json_encode(["success" => $stmt->execute()]);
    }
}

// CRUD CATALOGO MEDICO
elseif ($action == 'crud_catalogo') {
    $input = json_decode(file_get_contents('php://input'), true);
    if(isset($input['delete'])) {
        $stmt = $conn->prepare("DELETE FROM catalogo_medico WHERE id_medicamento=?");
        $stmt->bind_param("i", $input['id']);
        echo json_encode(["success" => $stmt->execute()]);
    } else {
        $nombre = $input['nombre'];
        $tipo = $input['tipo'];
        $dosis = floatval($input['dosis']);
        if(isset($input['id']) && $input['id']) {
            $stmt = $conn->prepare("UPDATE catalogo_medico SET nombre_producto=?, tipo_producto=?, dosis_mg_por_litro=? WHERE id_medicamento=?");
            $stmt->bind_param("ssdi", $nombre, $tipo, $dosis, $input['id']);
        } else {
            $stmt = $conn->prepare("INSERT INTO catalogo_medico (nombre_producto, tipo_producto, dosis_mg_por_litro) VALUES (?, ?, ?)");
            $stmt->bind_param("ssd", $nombre, $tipo, $dosis);
        }
        echo json_encode(["success" => $stmt->execute()]);
    }
}

// CRUD PROTOCOLOS
elseif ($action == 'crud_protocolo') {
    $input = json_decode(file_get_contents('php://input'), true);
    if(isset($input['delete'])) {
        $stmt = $conn->prepare("DELETE FROM protocolos_sanitarios WHERE id_protocolo=?");
        $stmt->bind_param("i", $input['id']);
        echo json_encode(["success" => $stmt->execute()]);
    } else {
        $esp = intval($input['id_especie']);
        $med = intval($input['id_medicamento']);
        $frec = intval($input['frecuencia']);
        $notas = $input['notas'];
        if(isset($input['id']) && $input['id']) {
            $stmt = $conn->prepare("UPDATE protocolos_sanitarios SET id_especie=?, id_medicamento=?, frecuencia_dias=?, notas_aplicacion=? WHERE id_protocolo=?");
            $stmt->bind_param("iiisi", $esp, $med, $frec, $notas, $input['id']);
        } else {
            $stmt = $conn->prepare("INSERT INTO protocolos_sanitarios (id_especie, id_medicamento, frecuencia_dias, notas_aplicacion) VALUES (?, ?, ?, ?)");
            $stmt->bind_param("iiis", $esp, $med, $frec, $notas);
        }
        echo json_encode(["success" => $stmt->execute()]);
    }
}

// CRUD CALENDARIO
elseif ($action == 'crud_calendario') {
    $input = json_decode(file_get_contents('php://input'), true);
    if(isset($input['delete'])) {
        $stmt = $conn->prepare("DELETE FROM calendario_sanitario WHERE id_evento=?");
        $stmt->bind_param("i", $input['id']);
        echo json_encode(["success" => $stmt->execute()]);
    } else {
        $est = intval($input['id_estanque']);
        $med = intval($input['id_medicamento']);
        $fecha_prog = $input['fecha_programada'];
        $estado = $input['estado'];
        $fecha_real = !empty($input['fecha_real']) ? $input['fecha_real'] : null;
        $dosis = !empty($input['dosis']) ? floatval($input['dosis']) : null;
        if(isset($input['id']) && $input['id']) {
            $stmt = $conn->prepare("UPDATE calendario_sanitario SET id_estanque=?, id_medicamento=?, fecha_programada=?, estado=?, fecha_aplicacion_real=?, dosis_total_aplicada_mg=? WHERE id_evento=?");
            $stmt->bind_param("iisssdi", $est, $med, $fecha_prog, $estado, $fecha_real, $dosis, $input['id']);
        } else {
            $stmt = $conn->prepare("INSERT INTO calendario_sanitario (id_estanque, id_medicamento, fecha_programada, estado, fecha_aplicacion_real, dosis_total_aplicada_mg) VALUES (?, ?, ?, ?, ?, ?)");
            $stmt->bind_param("iisssd", $est, $med, $fecha_prog, $estado, $fecha_real, $dosis);
        }
        echo json_encode(["success" => $stmt->execute()]);
    }
}

//SENSORES
if ($action == 'ultimas_lecturas') {
    $result = $conn->query("SELECT temp_agua, ph, turbidez_ntu as turbidez, nivel_agua_cm as nivel 
                            FROM lecturas ORDER BY fecha_hora DESC LIMIT 1");
    echo json_encode($result->fetch_assoc() ?: ["temp_agua"=>0, "ph"=>0, "turbidez"=>0, "nivel"=>0]);
}

$conn->close();
?>