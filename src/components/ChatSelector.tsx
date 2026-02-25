"use client";

import { motion } from "framer-motion";
import { TelegramChat } from "@/types";

interface Props {
  onSelect: (chat: TelegramChat) => void;
}

/**
 * Chat selector shown during onboarding.
 * In demo mode, shows sample chats. With a real Telegram bot integration,
 * this would fetch the user's actual chat list via the Bot API.
 */
export default function ChatSelector({ onSelect }: Props) {
  // Demo chats — replace with real Telegram Bot API integration
  const demoBots: TelegramChat[] = [
    {
      id: 1001,
      title: "OpenClaw Assistant",
      type: "private",
      bot_username: "openclaw_bot",
      last_message_date: Date.now() - 3600000,
    },
    {
      id: 1002,
      title: "CodeBuddy AI",
      type: "private",
      bot_username: "codebuddy_bot",
      last_message_date: Date.now() - 86400000,
    },
    {
      id: 1003,
      title: "StudyPal Bot",
      type: "private",
      bot_username: "studypal_bot",
      last_message_date: Date.now() - 172800000,
    },
    {
      id: 1004,
      title: "DailyMotivation",
      type: "private",
      bot_username: "motivation_bot",
      last_message_date: Date.now() - 259200000,
    },
  ];

  const container = {
    hidden: { opacity: 0 },
    show: {
      opacity: 1,
      transition: { staggerChildren: 0.1 },
    },
  };

  const item = {
    hidden: { opacity: 0, y: 20 },
    show: { opacity: 1, y: 0 },
  };

  return (
    <div className="w-full max-w-md">
      <h2 className="mb-2 text-center font-display text-xl font-bold text-gray-800">
        Choose your buddy&apos;s chat
      </h2>
      <p className="mb-6 text-center font-body text-sm text-gray-500">
        Pick which bot conversation your creature lives in
      </p>

      <motion.div
        className="flex flex-col gap-3"
        variants={container}
        initial="hidden"
        animate="show"
      >
        {demoBots.map((chat) => (
          <motion.button
            key={chat.id}
            variants={item}
            onClick={() => onSelect(chat)}
            className="flex items-center gap-4 rounded-2xl border-2 border-transparent bg-white p-4 text-left shadow-sm transition-all hover:border-lavender hover:shadow-md"
            whileHover={{ scale: 1.02 }}
            whileTap={{ scale: 0.98 }}
          >
            {/* Bot avatar */}
            <div className="flex h-12 w-12 items-center justify-center rounded-full bg-gradient-to-br from-lavender to-bubblegum text-xl">
              {getBotEmoji(chat.title)}
            </div>

            <div className="flex-1">
              <h3 className="font-display font-semibold text-gray-800">
                {chat.title}
              </h3>
              <p className="text-xs font-body text-gray-400">
                @{chat.bot_username}
              </p>
            </div>

            <div className="text-xs font-body text-gray-300">
              {getTimeAgo(chat.last_message_date)}
            </div>
          </motion.button>
        ))}
      </motion.div>

      <p className="mt-4 text-center text-xs font-body text-gray-400">
        Your creature&apos;s mood will reflect how often you chat here!
      </p>
    </div>
  );
}

function getBotEmoji(title: string): string {
  if (title.toLowerCase().includes("code")) return "🤖";
  if (title.toLowerCase().includes("study")) return "📚";
  if (title.toLowerCase().includes("motiv")) return "🌟";
  if (title.toLowerCase().includes("claw")) return "🦀";
  return "💬";
}

function getTimeAgo(timestamp?: number): string {
  if (!timestamp) return "";
  const diff = Date.now() - timestamp;
  const hours = Math.floor(diff / 3600000);
  if (hours < 1) return "just now";
  if (hours < 24) return `${hours}h ago`;
  const days = Math.floor(hours / 24);
  return `${days}d ago`;
}
