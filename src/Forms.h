const char LOGIN_FORM[] = R"rawhtml(
		<!DOCTYPE html>
		<html>
		<head>
				<meta name="viewport" content="width=device-width, initial-scale=1.0">
				<title>ESP12 Wi-Fi Setup</title>
				<style>
						body { font-family: Arial, sans-serif; margin: 20px; background-color: #f4f4f9; color: #333; }
						.container { max-width: 400px; margin: 0 auto; background: white; padding: 25px; border-radius: 8px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }
						h2 { margin-top: 0; color: #0076ff; text-align: center; }
						label { display: block; margin: 12px 0 6px; font-weight: bold; }
						input[type="text"], input[type="password"] { width: 100%; padding: 10px; box-sizing: border-box; border: 1px solid #ccc; border-radius: 4px; }
						input[type="submit"] { width: 100%; padding: 12px; margin-top: 20px; background-color: #0076ff; border: none; border-radius: 4px; color: white; font-size: 16px; cursor: pointer; }
						input[type="submit"]:hover { background-color: #0056b3; }
				</style>
		</head>
		<body>
				<div class="container">
						<h2>Wi-Fi Configuration</h2>
						<form action="/save" method="POST">
								<label for="ssid">Network Name (SSID):</label>
								<input type="text" id="ssid" name="ssid" placeholder="Enter Wi-Fi Name" required>
								<label for="password">Password:</label>
								<input type="password" id="password" name="password" placeholder="Enter Wi-Fi Password">
								<input type="submit" value="Save & Connect">
						</form>
				</div>
		</body>
		</html>
		)rawhtml";
