<?php
// page.php
session_start();

// Check if user is logged in (session and cookie)
if (!isset($_SESSION['username']) || !isset($_COOKIE['belhamid_omar'])) {
    // Redirect to login if not authenticated
    echo '<script>window.location.href="http://localhost:3434/cgi-bin/session_cookie/login.php";</script>';
    exit();
}

// Get username from session
$username = $_SESSION['username'];
?>

<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Welcome Page</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #f8f9fa;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            margin: 0;
        }
        .welcome-container {
            background: white;
            padding: 40px;
            border-radius: 10px;
            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
            text-align: center;
            min-width: 400px;
        }
        .welcome-container h1 {
            color: #28a745;
            margin-bottom: 20px;
        }
        .username {
            font-size: 24px;
            color: #007bff;
            font-weight: bold;
            margin-bottom: 30px;
        }
        .logout-btn {
            background-color: #dc3545;
            color: white;
            padding: 10px 20px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            text-decoration: none;
            display: inline-block;
        }
        .logout-btn:hover {
            background-color: #c82333;
        }
        .session-info {
            background-color: #e9ecef;
            padding: 15px;
            border-radius: 5px;
            margin-top: 20px;
            text-align: left;
        }
        .session-info h3 {
            margin-top: 0;
            color: #495057;
        }
    </style>
</head>
<body>
    <div class="welcome-container">
        <h1>Welcome!</h1>
        <div class="username">Hello <?php echo htmlspecialchars($username); ?>!</div>
        
        <div class="session-info">
            <h3>Session Information:</h3>
            <p><strong>Username:</strong> <?php echo htmlspecialchars($_SESSION['username']); ?></p>
            <p><strong>Session Status:</strong> Active</p>
            <p><strong>Cookie Set:</strong> Yes</p>
            <p><strong>Login Time:</strong> <?php echo date('Y-m-d H:i:s'); ?></p>
        </div>
        
        <br>
        <a href="http://localhost:3434/cgi-bin/session_cookie/logout.php" class="logout-btn">Logout</a>
    </div>
</body>
</html>