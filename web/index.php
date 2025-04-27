<?php
// --- Consulta a Supabase ---
$url = 'https://kduoudmniqjtartllozn.supabase.co/rest/v1/access_logs';
$apiKey = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImtkdW91ZG1uaXFqdGFydGxsb3puIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NDM5MzUwNzcsImV4cCI6MjA1OTUxMTA3N30.T1mpTXZaGF8AIzvQ96jeQh95Vo4cfLCiWh0QSH0Ea84';

$ch = curl_init($url);
curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
curl_setopt($ch, CURLOPT_HTTPHEADER, [
    'apikey: ' . $apiKey,
    'Authorization: Bearer ' . $apiKey,
    'Content-Type: application/json'
]);

$response = curl_exec($ch);
curl_close($ch);

$data = json_decode($response, true);
?>

<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <title>Dashboard - Fichaje</title>
    <script src="https://cdn.tailwindcss.com"></script>
</head>
<body class="bg-gray-100 flex">

<!-- Sidebar -->
<?php include 'includes/sidebar.php'; ?>

<!-- Contenido -->
<div class="ml-64 p-6 w-full">
    <h1 class="text-3xl font-bold mb-6">Registros de Fichaje</h1>

    <div class="bg-white shadow-md rounded-lg p-4 overflow-x-auto">
        <table class="min-w-full text-left">
            <thead>
                <tr class="bg-gray-200">
                    <th class="py-2 px-4">ID</th>
                    <th class="py-2 px-4">Hora de acceso</th>
                    <th class="py-2 px-4">Empleado ID</th>
                    <th class="py-2 px-4">Localización ID</th>
                    <th class="py-2 px-4">Método</th>
                    <th class="py-2 px-4">Fingerprint ID</th>
                    <th class="py-2 px-4">RFID Card</th>
                </tr>
            </thead>
            <tbody>
                <?php if (!empty($data)): ?>
                    <?php foreach($data as $row): ?>
                    <tr class="border-b hover:bg-gray-100">
                        <td class="py-2 px-4"><?= htmlspecialchars($row['id']) ?></td>
                        <td class="py-2 px-4">
                            <?= isset($row['access_time']) ? date('d/m/Y H:i', strtotime($row['access_time'])) : '' ?>
                        </td>
                        <td class="py-2 px-4"><?= htmlspecialchars($row['employee_id']) ?></td>
                        <td class="py-2 px-4"><?= htmlspecialchars($row['location_id']) ?></td>
                        <td class="py-2 px-4"><?= htmlspecialchars($row['access_method']) ?></td>
                        <td class="py-2 px-4"><?= htmlspecialchars($row['fingerprint_id']) ?></td>
                        <td class="py-2 px-4"><?= htmlspecialchars($row['rfid_card_code']) ?></td>
                    </tr>
                    <?php endforeach; ?>
                <?php else: ?>
                    <tr>
                        <td colspan="7" class="py-4 text-center">No hay registros de acceso.</td>
                    </tr>
                <?php endif; ?>
            </tbody>
        </table>
    </div>
</div>
</body>
</html>