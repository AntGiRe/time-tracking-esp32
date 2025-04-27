<div class="w-64 h-screen bg-gray-800 text-white fixed">
    <div class="p-6 text-2xl font-bold">Dashboard</div>
    <nav class="mt-6">
        <ul>
            <li class="p-3 hover:bg-gray-700 <?= basename($_SERVER['PHP_SELF']) == 'index.php' ? 'bg-gray-700' : '' ?>">
                <a href="index.php">Index</a>
            </li>
            <li class="p-3 hover:bg-gray-700 <?= basename($_SERVER['PHP_SELF']) == 'locations.php' ? 'bg-gray-700' : '' ?>">
                <a href="locations.php">Localizaciones</a>
            </li>
            <li class="p-3 hover:bg-gray-700 <?= basename($_SERVER['PHP_SELF']) == 'users.php' ? 'bg-gray-700' : '' ?>">
                <a href="users.php">Usuarios</a>
            </li>
        </ul>
    </nav>
</div>