"use client";

import { motion } from "framer-motion";

interface Props {
  message: string;
  isBot: boolean;
  timestamp?: string;
}

export default function ChatBubble({ message, isBot, timestamp }: Props) {
  return (
    <motion.div
      initial={{ opacity: 0, y: 10, scale: 0.95 }}
      animate={{ opacity: 1, y: 0, scale: 1 }}
      className={`flex ${isBot ? "justify-start" : "justify-end"}`}
    >
      <div
        className={`max-w-[80%] rounded-2xl px-4 py-2.5 ${
          isBot
            ? "rounded-bl-md bg-white text-gray-700 shadow-sm"
            : "rounded-br-md bg-lavender text-white shadow-sm"
        }`}
      >
        <p className="font-body text-sm leading-relaxed">{message}</p>
        {timestamp && (
          <p
            className={`mt-1 text-[10px] ${
              isBot ? "text-gray-300" : "text-white/60"
            }`}
          >
            {timestamp}
          </p>
        )}
      </div>
    </motion.div>
  );
}
