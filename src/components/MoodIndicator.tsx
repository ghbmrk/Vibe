"use client";

import { motion } from "framer-motion";
import { CreatureMood, MOOD_COLORS } from "@/types";

interface Props {
  mood: CreatureMood;
  happiness: number;
}

const MOOD_EMOJI: Record<CreatureMood, string> = {
  ecstatic: "🤩",
  happy: "😊",
  content: "☺️",
  neutral: "😐",
  lonely: "🥺",
  sad: "😢",
  crying: "😭",
};

const MOOD_LABELS: Record<CreatureMood, string> = {
  ecstatic: "Ecstatic!",
  happy: "Happy",
  content: "Content",
  neutral: "Neutral",
  lonely: "Lonely",
  sad: "Sad",
  crying: "Very Sad",
};

export default function MoodIndicator({ mood, happiness }: Props) {
  const color = MOOD_COLORS[mood];

  return (
    <div className="flex items-center gap-3 rounded-2xl bg-white/80 px-4 py-2 shadow-sm backdrop-blur-sm">
      <motion.span
        className="text-2xl"
        animate={{ scale: [1, 1.15, 1] }}
        transition={{ duration: 2, repeat: Infinity }}
      >
        {MOOD_EMOJI[mood]}
      </motion.span>

      <div className="flex flex-col gap-1">
        <span className="text-sm font-display font-semibold text-gray-700">
          {MOOD_LABELS[mood]}
        </span>
        <div className="h-1.5 w-20 overflow-hidden rounded-full bg-gray-100">
          <motion.div
            className="h-full rounded-full"
            style={{ backgroundColor: color }}
            initial={false}
            animate={{ width: `${happiness}%` }}
            transition={{ duration: 0.6 }}
          />
        </div>
      </div>
    </div>
  );
}
