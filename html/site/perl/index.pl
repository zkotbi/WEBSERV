#!/usr/bin/perl
use strict;
use CGI;
use CGI::Cookie;



my $cgi = CGI->new;
# Get existing cookies
my %cookies = fetch CGI::Cookie;
my $current_theme = $cookies{'theme'} ? $cookies{'theme'}->value : 'white';

my $status_code = 200;  # Example: setting 404 status code for "Not Found"
my $status_message = "OK";


# Handle form submission
if ($cgi->param('theme')) {
    # Create new cookie
    my $cookie = CGI::Cookie->new(
        -name    => 'theme',
        -value   => $cgi->param('theme'),
        -expires => '+1M'  # Cookie expires in 1 month
    );
    print $cgi->header(
		-cookie => $cookie
	);
    $current_theme = $cgi->param('theme');
} else {

    print $cgi->header(
		-status  => "$status_code $status_message",  # Set status code and message
	);
}

# Define theme colors
my %theme_colors = (
    'blue'   => '#e6f3ff',
    'green'  => '#e6ffe6',
    'pink'   => '#ffe6f0',
    'yellow' => '#fffff0',
    'white'  => '#ffffff'
);

# Print HTML
print <<HTML;
<!DOCTYPE html>
<html>
<head>
    <title>Theme Selector</title>
    <style>
        body { 
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 0;
            padding-top: 100px;
            background-color: $theme_colors{$current_theme};
            transition: background-color 0.5s ease;
            display: flex;
            justify-content: center;
            align-items: start;
            height: 94vh;
            text-align: center;
        }
        .theme-box {
            background: white;
            padding: 30px;
            border-radius: 8px;
            box-shadow: 0 0 10px rgba(0,0,0,0.1);
            width: 300px;
        }
        h1 {
            margin-top: 0;
            color: #333;
        }
        select {
            padding: 10px;
            margin: 15px 0;
            width: 80%;
            text-align-last: center;
            border-radius: 4px;
            border: 1px solid #ddd;
        }
        input[type="submit"] {
            background: #4CAF50;
            color: white;
            padding: 10px 20px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            width: 80%;
            font-size: 16px;
            transition: background-color 0.3s;
        }
        input[type="submit"]:hover {
            background: #45a049;
        }
        p {
            color: #666;
            margin: 15px 0;
        }
    </style>
</head>
<body>
    <div class="theme-box">
        <h1>Theme Selector</h1>
        <p>Current theme: $current_theme</p>
        <form method="POST">
            <select name="theme">
                <option value="blue" @{[$current_theme eq 'blue' ? 'selected' : '']}>Blue Theme</option>
                <option value="green" @{[$current_theme eq 'green' ? 'selected' : '']}>Green Theme</option>
                <option value="pink" @{[$current_theme eq 'pink' ? 'selected' : '']}>Pink Theme</option>
                <option value="yellow" @{[$current_theme eq 'yellow' ? 'selected' : '']}>Yellow Theme</option>
                <option value="white" @{[$current_theme eq 'white' ? 'selected' : '']}>Default White</option>
            </select>
            <br>
            <input type="submit" value="Change Theme">
        </form>
    </div>
</body>
</html>
HTML
