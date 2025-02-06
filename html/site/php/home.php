<?php
session_start();
if (!isset($_SESSION['user_id'])) {
    header('Location: login.php');
	http_response_code(302);
    exit;
}

// Get user details from database
try {
    $db = new SQLite3('/Users/hibenouk/database.db');
    $stmt = $db->prepare('SELECT full_name, username FROM users WHERE id = :id');
    $stmt->bindValue(':id', $_SESSION['user_id'], SQLITE3_INTEGER);
    $result = $stmt->execute();
    $user = $result->fetchArray(SQLITE3_ASSOC);
} catch (Exception $e) {
    die('Database error: ' . $e->getMessage());
}
?>

<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Welcome - Dashboard</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: 'Arial', sans-serif;
            min-height: 100vh;
            background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
            padding: 20px;
        }

        .navbar {
            background-color: white;
            padding: 15px 30px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            display: flex;
            justify-content: space-between;
            align-items: center;
            border-radius: 10px;
            margin-bottom: 30px;
        }

        .logo {
            font-size: 24px;
            font-weight: bold;
            color: #2c3e50;
        }

        .logout-btn {
            padding: 8px 20px;
            background-color: #e74c3c;
            color: white;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            text-decoration: none;
            font-size: 14px;
            transition: background-color 0.3s;
        }

        .logout-btn:hover {
            background-color: #c0392b;
        }

        .welcome-container {
            background-color: white;
            border-radius: 15px;
            padding: 40px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
            max-width: 800px;
            margin: 0 auto;
            text-align: center;
        }

        .welcome-header {
            margin-bottom: 30px;
            color: #2c3e50;
            font-size: 2.5em;
            font-weight: bold;
        }

        .user-info {
            background-color: #f8f9fa;
            padding: 30px;
            border-radius: 10px;
            margin-top: 30px;
        }

        .user-detail {
            margin: 15px 0;
            padding: 15px;
            background-color: white;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.05);
        }

        .detail-label {
            color: #7f8c8d;
            font-size: 0.9em;
            margin-bottom: 5px;
            text-transform: uppercase;
            letter-spacing: 1px;
        }

        .detail-value {
            color: #2c3e50;
            font-size: 1.2em;
            font-weight: 500;
        }

        .welcome-message {
            font-size: 1.2em;
            color: #34495e;
            line-height: 1.6;
            margin: 20px 0;
            padding: 20px;
            border-left: 4px solid #3498db;
            background-color: #f8f9fa;
            text-align: left;
        }

        .time-greeting {
            color: #3498db;
            font-weight: bold;
        }
    </style>
</head>
<body>
    <nav class="navbar">
        <div class="logo">MyApp</div>
        <a href="logout.php" class="logout-btn">Logout</a>
    </nav>

    <div class="welcome-container">
        <h1 class="welcome-header">
            <?php
            $hour = date('H');
            $greeting = '';
            
            if ($hour < 12) {
                $greeting = 'Good Morning';
            } elseif ($hour < 18) {
                $greeting = 'Good Afternoon';
            } else {
                $greeting = 'Good Evening';
            }
            ?>
            <span class="time-greeting"><?php echo $greeting; ?></span>
        </h1>

        <div class="welcome-message">
            Welcome back to your dashboard! We're glad to see you again. Here you can manage your account and access all your personal information.
        </div>

        <div class="user-info">
            <div class="user-detail">
                <div class="detail-label">Full Name</div>
                <div class="detail-value"><?php echo htmlspecialchars($user['full_name']); ?></div>
            </div>

            <div class="user-detail">
                <div class="detail-label">Username</div>
                <div class="detail-value"><?php echo htmlspecialchars($user['username']); ?></div>
            </div>
        </div>
    </div>
</body>
</html>
