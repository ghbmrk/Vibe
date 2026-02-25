"use client";

import { motion } from "framer-motion";
import { CreatureState } from "@/types";

interface Props {
  creature: CreatureState;
}

export default function StatsPanel({ creature }: Props) {
  const stats = [
    {
      label: "Messages",
      value: creature.totalMessages,
      icon: "💬",
      color: "bg-lavender/20 text-purple-600",
    },
    {
      label: "Streak",
      value: `${creature.streak}d`,
      icon: "🔥",
      color: "bg-orange-100 text-orange-600",
    },
    {
      label: "Level",
      value: creature.level,
      icon: "⭐",
      color: "bg-sunshine/20 text-yellow-600",
    },
    {
      label: "Mood",
      value: `${creature.happiness}%`,
      icon: "💕",
      color: "bg-bubblegum/20 text-pink-600",
    },
  ];

  return (
    <div className="grid grid-cols-4 gap-2 sm:gap-3">
      {stats.map((stat, i) => (
        <motion.div
          key={stat.label}
          initial={{ opacity: 0, y: 20 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ delay: i * 0.1 }}
          className={`flex flex-col items-center rounded-2xl ${stat.color} px-3 py-3 backdrop-blur-sm`}
        >
          <span className="text-lg">{stat.icon}</span>
          <span className="font-display text-lg font-bold">{stat.value}</span>
          <span className="text-[10px] font-body opacity-70">{stat.label}</span>
        </motion.div>
      ))}
    </div>
  );
}
