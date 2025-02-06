<?php
// [Previous PHP code remains exactly the same until the HTML part]
session_start();
if (isset($_SESSION['user_id'])) {
    header('Location: home.php');
    exit;
}

// Database connection
class Database {
    private $db;

    public function __construct() {
        try {
            $this->db = new SQLite3('/Users/hibenouk/database.db');
            // Create users table if it doesn't exist
        } catch (Exception $e) {
            die('Database connection failed: ' . $e->getMessage());
        }
    }

    public function getDb() {
        return $this->db;
    }
}

// User authentication class
class Auth {
    private $db;

    public function __construct(Database $database) {
        $this->db = $database->getDb();
    }

    public function login($username, $password) {
        // Prepare statement to prevent SQL injection
        $stmt = $this->db->prepare('
            SELECT id, password 
            FROM users 
            WHERE username = :username
        ');
        
        $stmt->bindValue(':username', $username, SQLITE3_TEXT);
        $result = $stmt->execute();
        
        if ($user = $result->fetchArray(SQLITE3_ASSOC)) {
            // Verify password hash
            if (password_verify($password, $user['password'])) {
                $_SESSION['user_id'] = $user['id'];
                $_SESSION['username'] = $username;
                return true;
            }
        }
        return false;
    }
}

// Process login form
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $auth = new Auth(new Database());
    $pass = $_POST['password'] ?? null;
    $user = $_POST['username'] ?? null;

    if ($user && $pass &&  $auth->login($user, $pass)) {
        header('Location: home.php');
        exit;
    } else {
        $error = "Invalid username or password";
    }
}
?>

<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Login</title>
    <style>
        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }

        body {
            font-family: Arial, sans-serif;
            display: flex;
            flex-direction: column;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
            background-color: #f5f5f5;
            padding: 20px;
        }

        .main-title {
            font-size: 2.5rem;
            color: #2c3e50;
            margin-bottom: 30px;
            text-align: center;
            font-weight: bold;
            letter-spacing: 2px;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.1);
            animation: fadeIn 1s ease-in;
        }

        @keyframes fadeIn {
            from {
                opacity: 0;
                transform: translateY(-20px);
            }
            to {
                opacity: 1;
                transform: translateY(0);
            }
        }

        .login-container {
            background-color: white;
            padding: 30px;
            border-radius: 5px;
            box-shadow: 0 0 10px rgba(0,0,0,0.1);
            width: 100%;
            max-width: 400px;
            margin: 0 20px;
        }

        h2 {
            text-align: center;
            margin-bottom: 20px;
            color: #333;
        }

        .form-group {
            margin-bottom: 20px;
            width: 100%;
        }

        label {
            display: block;
            margin-bottom: 8px;
            color: #555;
            font-weight: 500;
        }

        input {
            width: 100%;
            padding: 10px;
            border: 1px solid #ddd;
            border-radius: 4px;
            font-size: 16px;
            outline: none;
            transition: border-color 0.3s;
        }

        input:focus {
            border-color: #007bff;
        }

        button {
            width: 100%;
            padding: 12px;
            background-color: #007bff;
            color: white;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 16px;
            transition: background-color 0.3s;
        }

        button:hover {
            background-color: #0056b3;
        }

        .error {
            color: #dc3545;
            margin-bottom: 15px;
            text-align: center;
            font-size: 14px;
            padding: 10px;
            background-color: #f8d7da;
            border-radius: 4px;
        }

        .login-link {
            text-align: center;
            margin-top: 20px;
        }

        .login-link a {
            color: #007bff;
            text-decoration: none;
        }

        .login-link a:hover {
            text-decoration: underline;
        }
    </style>
</head>
<body>
    <h1 class="main-title">Note App</h1>
    <div class="login-container">
        <h2>Login</h2>
        <?php if (isset($error)): ?>
            <div class="error"><?php echo htmlspecialchars($error); ?></div>
        <?php endif; ?>
        <form method="POST" action="">
            <div class="form-group">
                <label for="username">Username:</label>
                <input type="text" id="username" name="username" required>
            </div>
            <div class="form-group">
                <label for="password">Password:</label>
                <input type="password" id="password" name="password" required>
            </div>
            <button type="submit">Login</button>
        </form>
        <div class="login-link">
            New Users? <a href="register.php">Create Account</a>
        </div>
    </div>
</body>
</html>
