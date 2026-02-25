import type { Metadata, Viewport } from "next";
import "./globals.css";

export const metadata: Metadata = {
  title: "Vibe — Your Tamagotchi Chat Buddy",
  description:
    "A cute tamagotchi companion that lives in your Telegram chats. Keep it happy by chatting!",
  icons: { icon: "/favicon.ico" },
};

export const viewport: Viewport = {
  width: "device-width",
  initialScale: 1,
  maximumScale: 1,
  userScalable: false,
  themeColor: "#FFF8F0",
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html lang="en">
      <head>
        <link
          href="https://fonts.googleapis.com/css2?family=Nunito:wght@400;600;700;800&family=Quicksand:wght@400;500;600;700&display=swap"
          rel="stylesheet"
        />
      </head>
      <body className="min-h-screen antialiased">
        <div className="relative mx-auto min-h-screen max-w-lg">
          {/* Background particles */}
          <div className="pointer-events-none fixed inset-0 overflow-hidden">
            <div
              className="particle bg-bubblegum/20"
              style={{
                width: 60,
                height: 60,
                top: "10%",
                left: "15%",
                animationDelay: "0s",
              }}
            />
            <div
              className="particle bg-lavender/20"
              style={{
                width: 40,
                height: 40,
                top: "30%",
                right: "10%",
                animationDelay: "2s",
              }}
            />
            <div
              className="particle bg-mint/20"
              style={{
                width: 50,
                height: 50,
                bottom: "20%",
                left: "20%",
                animationDelay: "4s",
              }}
            />
            <div
              className="particle bg-sunshine/20"
              style={{
                width: 35,
                height: 35,
                top: "60%",
                right: "25%",
                animationDelay: "1s",
              }}
            />
            <div
              className="particle bg-sky/20"
              style={{
                width: 45,
                height: 45,
                bottom: "40%",
                left: "60%",
                animationDelay: "3s",
              }}
            />
          </div>

          {children}
        </div>
      </body>
    </html>
  );
}
