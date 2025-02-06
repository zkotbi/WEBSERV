<?php
session_start();
if (isset($_SESSION['user_id'])) {
	header('Location: home.php');
	exit;
}

class Database
{
	private $db;

	public function __construct()
	{
		try {
			$this->db = new SQLite3('/Users/hibenouk/database.db');
			// Update users table to include full_name
			// $this->db->exec();
		} catch (Exception $e) {
			die('Database connection failed: ' . $e->getMessage());
		}
	}

	public function getDb()
	{
		return $this->db;
	}
}

class Registration
{
	private $db;
	private $errors = [];

	public function __construct(Database $database)
	{
		$this->db = $database->getDb();
	}

	public function validate($fullName, $username, $password, $confirmPassword)
	{
		// Validate full name
		if (strlen($fullName) < 2) {
			$this->errors['fullName'] = "Full name must be at least 2 characters long";
		}

		// Validate username
		if (strlen($username) < 3) {
			$this->errors['username'] = "Username must be at least 3 characters long";
		}

		// Check if username already exists
		$stmt = $this->db->prepare('SELECT username FROM users WHERE username = :username');
		$stmt->bindValue(':username', $username, SQLITE3_TEXT);
		$result = $stmt->execute();
		if ($result->fetchArray()) {
			$this->errors['username'] = "Username already exists";
		}

		// Validate password
		if (strlen($password) < 6) {
			$this->errors['password'] = "Password must be at least 6 characters long";
		}

		// Confirm passwords match
		if ($password !== $confirmPassword) {
			$this->errors['confirmPassword'] = "Passwords do not match";
		}

		return empty($this->errors);
	}

	public function register($fullName, $username, $password)
	{
		$hashedPassword = password_hash($password, PASSWORD_DEFAULT);

		$stmt = $this->db->prepare('
            INSERT INTO users (full_name, username, password)
            VALUES (:full_name, :username, :password)
        ');

		$stmt->bindValue(':full_name', $fullName, SQLITE3_TEXT);
		$stmt->bindValue(':username', $username, SQLITE3_TEXT);
		$stmt->bindValue(':password', $hashedPassword, SQLITE3_TEXT);

		return $stmt->execute();
	}

	public function getErrors()
	{
		return $this->errors;
	}
}

// Process registration form
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
	$registration = new Registration(new Database());
	$fullName = htmlspecialchars($_POST['full_name'], ENT_QUOTES, 'UTF-8') ?? "";
	$username = htmlspecialchars($_POST['username'], ENT_QUOTES, 'UTF-8') ?? "";
	$password = $_POST['password'] ?? "";
	$confirmPassword = $_POST['confirm_password'] ?? "";

	if ($registration->validate($fullName, $username, $password, $confirmPassword)) {
		if ($registration->register($fullName, $username, $password)) {
			$_SESSION['success'] = "Account created successfully! You can now login.";
			header('Location: login.php');
			exit;
		} else {
			$error = "Registration failed. Please try again.";
		}
	} else {
		$errors = $registration->getErrors();
	}
}
?>

<!DOCTYPE html>
<html lang="en">

<head>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<title>Create Account</title>
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
			background-color: #f5f5f5;
			padding: 20px;
		}

		.main-title {
			font-size: 2.5rem;
			color: #2c3e50;
			margin-bottom: 30px;
			text-align: center;
			font-weight: bold;
			text-transform: uppercase;
			letter-spacing: 2px;
			text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.1);
		}

		.register-container {
			background-color: white;
			padding: 30px;
			border-radius: 5px;
			box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
			width: 100%;
			max-width: 400px;
		}

		h2 {
			text-align: center;
			margin-bottom: 20px;
			color: #333;
		}

		.form-group {
			margin-bottom: 20px;
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
			font-size: 14px;
			margin-top: 5px;
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
	<h1 class="main-title">Create Account</h1>
	<div class="register-container">
		<h2>Sign Up</h2>
		<?php if (isset($error)): ?>
			<div class="error"><?php echo htmlspecialchars($error); ?></div>
		<?php endif; ?>

		<form method="POST" action="">
			<div class="form-group">
				<label for="full_name">Full Name:</label>
				<input type="text" id="full_name" name="full_name" required
					value="<?php echo isset($fullName) ? htmlspecialchars($fullName) : ''; ?>">
				<?php if (isset($errors['fullName'])): ?>
					<div class="error"><?php echo htmlspecialchars($errors['fullName']); ?></div>
				<?php endif; ?>
			</div>

			<div class="form-group">
				<label for="username">Username:</label>
				<input type="text" id="username" name="username" required
					value="<?php echo isset($username) ? htmlspecialchars($username) : ''; ?>">
				<?php if (isset($errors['username'])): ?>
					<div class="error"><?php echo htmlspecialchars($errors['username']); ?></div>
				<?php endif; ?>
			</div>

			<div class="form-group">
				<label for="password">Password:</label>
				<input type="password" id="password" name="password" required>
				<?php if (isset($errors['password'])): ?>
					<div class="error"><?php echo htmlspecialchars($errors['password']); ?></div>
				<?php endif; ?>
			</div>

			<div class="form-group">
				<label for="confirm_password">Confirm Password:</label>
				<input type="password" id="confirm_password" name="confirm_password" required>
				<?php if (isset($errors['confirmPassword'])): ?>
					<div class="error"><?php echo htmlspecialchars($errors['confirmPassword']); ?></div>
				<?php endif; ?>
			</div>

			<button type="submit">Create Account</button>
		</form>

		<div class="login-link">
			Already have an account? <a href="login.php">Login here</a>
		</div>
	</div>
</body>

</html>
