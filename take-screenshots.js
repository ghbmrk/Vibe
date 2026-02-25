const puppeteer = require('puppeteer-core');

(async () => {
  const browser = await puppeteer.launch({
    executablePath: '/root/.cache/ms-playwright/chromium-1194/chrome-linux/chrome',
    headless: 'new',
    args: ['--no-sandbox', '--disable-setuid-sandbox', '--disable-gpu']
  });

  const page = await browser.newPage();
  await page.setViewport({ width: 390, height: 844, deviceScaleFactor: 2 });

  // Load the preview
  await page.goto('file:///home/user/Vibe/public/preview.html', { waitUntil: 'networkidle0', timeout: 15000 });
  await new Promise(r => setTimeout(r, 1500));

  // Screenshot 1: Login screen
  await page.screenshot({ path: '/home/user/Vibe/screenshot-1-login.png', fullPage: false });
  console.log('Screenshot 1: Login');

  // Screenshot 2: Click Telegram button -> Chat selector
  await page.click('.telegram-btn');
  await new Promise(r => setTimeout(r, 800));
  await page.screenshot({ path: '/home/user/Vibe/screenshot-2-chat-selector.png', fullPage: false });
  console.log('Screenshot 2: Chat Selector');

  // Screenshot 3: Click first chat -> Dashboard
  await page.click('.chat-item');
  await new Promise(r => setTimeout(r, 800));
  await page.screenshot({ path: '/home/user/Vibe/screenshot-3-dashboard.png', fullPage: false });
  console.log('Screenshot 3: Dashboard');

  // Screenshot 4: Type a message and send
  await page.type('#chat-input', 'Hello! How are you?');
  await page.click('.send-btn');
  await new Promise(r => setTimeout(r, 2000));
  await page.screenshot({ path: '/home/user/Vibe/screenshot-4-chat.png', fullPage: false });
  console.log('Screenshot 4: Chat with messages');

  await browser.close();
  console.log('Done!');
})();
