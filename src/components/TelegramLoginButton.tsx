"use client";

import { motion } from "framer-motion";
import { TelegramUser } from "@/types";
import { useEffect, useRef } from "react";

interface Props {
  botUsername: string;
  onAuth: (user: TelegramUser) => void;
  useDemoMode?: boolean;
}

export default function TelegramLoginButton({
  botUsername,
  onAuth,
  useDemoMode = false,
}: Props) {
  const containerRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    // Make callback available globally for Telegram widget
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    const win = window as any;
    win.__telegram_login_callback = (user: TelegramUser) => {
      onAuth(user);
    };

    // If we have a real bot username, inject the Telegram widget script
    if (!useDemoMode && botUsername && containerRef.current) {
      const script = document.createElement("script");
      script.src = "https://telegram.org/js/telegram-widget.js?22";
      script.setAttribute("data-telegram-login", botUsername);
      script.setAttribute("data-size", "large");
      script.setAttribute("data-radius", "20");
      script.setAttribute("data-onauth", "__telegram_login_callback(user)");
      script.setAttribute("data-request-access", "write");
      script.async = true;
      containerRef.current.appendChild(script);
    }

    return () => {
      delete win.__telegram_login_callback;
    };
  }, [botUsername, onAuth, useDemoMode]);

  const handleDemoLogin = () => {
    onAuth({
      id: 12345678,
      first_name: "Demo",
      last_name: "User",
      username: "demo_user",
      photo_url: undefined,
      auth_date: Math.floor(Date.now() / 1000),
      hash: "demo_hash",
    });
  };

  return (
    <div className="flex flex-col items-center gap-4">
      {/* Real Telegram widget container */}
      <div ref={containerRef} />

      {/* Demo / fallback button when no bot is configured */}
      {(useDemoMode || !botUsername) && (
        <motion.button
          onClick={handleDemoLogin}
          className="flex items-center gap-3 rounded-2xl bg-[#0088cc] px-8 py-4 font-display font-bold text-white shadow-lg shadow-blue-200/50 transition-colors hover:bg-[#0077b5]"
          whileHover={{ scale: 1.05 }}
          whileTap={{ scale: 0.95 }}
        >
          <TelegramIcon />
          Sign in with Telegram
        </motion.button>
      )}

      {(useDemoMode || !botUsername) && (
        <p className="text-xs font-body text-gray-400">
          Demo mode — set TELEGRAM_BOT_USERNAME for real login
        </p>
      )}
    </div>
  );
}

function TelegramIcon() {
  return (
    <svg width="24" height="24" viewBox="0 0 24 24" fill="currentColor">
      <path d="M11.944 0A12 12 0 0 0 0 12a12 12 0 0 0 12 12 12 12 0 0 0 12-12A12 12 0 0 0 12 0a12 12 0 0 0-.056 0zm4.962 7.224c.1-.002.321.023.465.14a.506.506 0 0 1 .171.325c.016.093.036.306.02.472-.18 1.898-.962 6.502-1.36 8.627-.168.9-.499 1.201-.82 1.23-.696.065-1.225-.46-1.9-.902-1.056-.693-1.653-1.124-2.678-1.8-1.185-.78-.417-1.21.258-1.91.177-.184 3.247-2.977 3.307-3.23.007-.032.014-.15-.056-.212s-.174-.041-.249-.024c-.106.024-1.793 1.14-5.061 3.345-.48.33-.913.49-1.302.48-.428-.008-1.252-.241-1.865-.44-.752-.245-1.349-.374-1.297-.789.027-.216.325-.437.893-.663 3.498-1.524 5.83-2.529 6.998-3.014 3.332-1.386 4.025-1.627 4.476-1.635z" />
    </svg>
  );
}
