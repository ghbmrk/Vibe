"use client";

import { motion } from "framer-motion";
import { TelegramUser } from "@/types";

interface Props {
  user: TelegramUser | null;
  onLogout?: () => void;
}

export default function Header({ user, onLogout }: Props) {
  return (
    <header className="flex items-center justify-between px-4 py-3 sm:px-6">
      <motion.div
        className="flex items-center gap-2"
        initial={{ opacity: 0, x: -20 }}
        animate={{ opacity: 1, x: 0 }}
      >
        <span className="text-2xl">🐾</span>
        <h1 className="font-display text-xl font-bold text-gray-800">
          Vibe
        </h1>
      </motion.div>

      {user && (
        <motion.div
          className="flex items-center gap-3"
          initial={{ opacity: 0, x: 20 }}
          animate={{ opacity: 1, x: 0 }}
        >
          <div className="flex items-center gap-2">
            {user.photo_url ? (
              <img
                src={user.photo_url}
                alt={user.first_name}
                className="h-8 w-8 rounded-full"
              />
            ) : (
              <div className="flex h-8 w-8 items-center justify-center rounded-full bg-lavender text-sm font-bold text-white">
                {user.first_name[0]}
              </div>
            )}
            <span className="hidden text-sm font-body font-medium text-gray-600 sm:block">
              {user.first_name}
            </span>
          </div>

          {onLogout && (
            <button
              onClick={onLogout}
              className="rounded-lg p-1.5 text-gray-400 transition-colors hover:bg-gray-100 hover:text-gray-600"
              title="Logout"
            >
              <svg
                width="16"
                height="16"
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                strokeWidth="2"
              >
                <path d="M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4" />
                <polyline points="16 17 21 12 16 7" />
                <line x1="21" y1="12" x2="9" y2="12" />
              </svg>
            </button>
          )}
        </motion.div>
      )}
    </header>
  );
}
