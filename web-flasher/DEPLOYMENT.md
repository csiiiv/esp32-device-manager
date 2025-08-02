# Deployment Guide

## Local Development

### Option 1: Simple HTTP Server (for testing without serial)

```bash
# Navigate to the web-flasher directory
cd web-flasher

# Start a simple HTTP server
python -m http.server 8000

# Open http://localhost:8000 in your browser
```

### Option 2: HTTPS Server (required for Web Serial API)

```bash
# Install mkcert for local HTTPS certificates
# Windows: choco install mkcert
# macOS: brew install mkcert
# Linux: sudo apt install mkcert

# Generate local certificates
mkcert -install
mkcert localhost

# Start HTTPS server with Python
python -m http.server 8000 --bind localhost

# Open https://localhost:8000 in your browser
```

### Option 3: Using Node.js

```bash
# Install serve globally
npm install -g serve

# Serve with HTTPS
serve -s . --ssl-cert localhost.pem --ssl-key localhost-key.pem

# Open the provided HTTPS URL in your browser
```

## GitHub Pages Deployment

### Step 1: Create GitHub Repository

1. Go to GitHub and create a new repository
2. Name it something like `esp32-web-flasher`
3. Make it public (required for GitHub Pages)

### Step 2: Upload Files

1. Clone the repository locally:
```bash
git clone https://github.com/yourusername/esp32-web-flasher.git
cd esp32-web-flasher
```

2. Copy all files from the `web-flasher` directory:
```bash
cp -r ../web-flasher/* .
```

3. Commit and push:
```bash
git add .
git commit -m "Initial commit: ESP32 Web Flasher"
git push origin main
```

### Step 3: Enable GitHub Pages

1. Go to your repository on GitHub
2. Click "Settings" tab
3. Scroll down to "Pages" section
4. Under "Source", select "Deploy from a branch"
5. Choose "main" branch and "/ (root)" folder
6. Click "Save"

Your web flasher will be available at:
`https://yourusername.github.io/esp32-web-flasher/`

## Netlify Deployment

### Step 1: Prepare Files

1. Create a new directory for deployment
2. Copy all files from `web-flasher` directory
3. Create a `netlify.toml` file:

```toml
[build]
  publish = "."
  command = ""

[[headers]]
  for = "/*"
  [headers.values]
    X-Frame-Options = "DENY"
    X-XSS-Protection = "1; mode=block"
    X-Content-Type-Options = "nosniff"
    Referrer-Policy = "strict-origin-when-cross-origin"
```

### Step 2: Deploy to Netlify

1. Go to [netlify.com](https://netlify.com)
2. Sign up/login with your GitHub account
3. Click "New site from Git"
4. Choose your repository
5. Configure build settings:
   - Build command: (leave empty)
   - Publish directory: `.`
6. Click "Deploy site"

## Vercel Deployment

### Step 1: Install Vercel CLI

```bash
npm install -g vercel
```

### Step 2: Deploy

```bash
# Navigate to web-flasher directory
cd web-flasher

# Deploy to Vercel
vercel

# Follow the prompts to configure your project
```

## Testing the Deployment

### 1. Basic Functionality Test

1. Open the deployed URL in Chrome/Edge
2. Check if the interface loads correctly
3. Verify all buttons and elements are visible
4. Test file upload functionality (without connecting device)

### 2. Serial API Test

1. Connect an ESP32 device via USB
2. Click "Connect Device"
3. Select the ESP32 from the port list
4. Verify connection status changes to "Connected"

### 3. Firmware Test

1. Prepare a test .bin firmware file
2. Upload the firmware file
3. Test the flashing process (with a test device)

## Troubleshooting Deployment

### Common Issues

1. **HTTPS Required**: Web Serial API only works over HTTPS
   - Solution: Use HTTPS deployment (GitHub Pages, Netlify, Vercel)

2. **CORS Issues**: If loading resources fails
   - Solution: Ensure all files are in the same directory
   - Check file paths in HTML

3. **File Not Found**: Missing files after deployment
   - Solution: Verify all files are uploaded
   - Check file permissions

4. **Serial API Not Available**: Browser compatibility
   - Solution: Use Chrome/Edge/Opera
   - Ensure HTTPS is enabled

### Performance Optimization

1. **Minify Files**: For production deployment
   ```bash
   # Install minification tools
   npm install -g uglify-js clean-css-cli

   # Minify JavaScript
   uglifyjs js/app.js -o js/app.min.js

   # Minify CSS
   cleancss css/style.css -o css/style.min.css
   ```

2. **Enable Compression**: Configure server compression
3. **Cache Headers**: Set appropriate cache headers for static files

## Security Considerations

1. **HTTPS Only**: Always use HTTPS for production
2. **Content Security Policy**: Add CSP headers if needed
3. **No Sensitive Data**: Don't store sensitive information in the web app
4. **Local Communication**: All serial communication is local to the user's machine

## Monitoring and Analytics

### Optional: Add Analytics

```html
<!-- Add to index.html head section -->
<script async src="https://www.googletagmanager.com/gtag/js?id=GA_MEASUREMENT_ID"></script>
<script>
  window.dataLayer = window.dataLayer || [];
  function gtag(){dataLayer.push(arguments);}
  gtag('js', new Date());
  gtag('config', 'GA_MEASUREMENT_ID');
</script>
```

### Error Tracking

Consider adding error tracking for production deployments:
- Sentry
- LogRocket
- Bugsnag

## Maintenance

### Regular Updates

1. **Check Dependencies**: Update esptool-js library periodically
2. **Browser Compatibility**: Test with new browser versions
3. **Security Updates**: Keep deployment platform updated

### Backup Strategy

1. **Version Control**: Use Git for all changes
2. **Multiple Deployments**: Consider staging environment
3. **Documentation**: Keep deployment notes updated 