"use client";

import { motion, AnimatePresence } from "framer-motion";
import { CreatureMood, CreatureState, MOOD_COLORS } from "@/types";
import { getMoodMessage } from "@/lib/mood";
import { useEffect, useState } from "react";

interface Props {
  state: CreatureState;
  size?: "sm" | "md" | "lg";
  onClick?: () => void;
}

const SIZE_MAP = { sm: 120, md: 200, lg: 280 };

export default function TamagotchiCreature({
  state,
  size = "md",
  onClick,
}: Props) {
  const [message, setMessage] = useState("");
  const [showMessage, setShowMessage] = useState(false);
  const px = SIZE_MAP[size];
  const color = MOOD_COLORS[state.mood];

  useEffect(() => {
    setMessage(getMoodMessage(state.mood));
    setShowMessage(true);
    const timer = setTimeout(() => setShowMessage(false), 4000);
    return () => clearTimeout(timer);
  }, [state.mood]);

  const handleClick = () => {
    setMessage(getMoodMessage(state.mood));
    setShowMessage(true);
    setTimeout(() => setShowMessage(false), 3000);
    onClick?.();
  };

  return (
    <div className="relative flex flex-col items-center" onClick={handleClick}>
      {/* Speech bubble */}
      <AnimatePresence>
        {showMessage && (
          <motion.div
            initial={{ opacity: 0, y: 10, scale: 0.8 }}
            animate={{ opacity: 1, y: 0, scale: 1 }}
            exit={{ opacity: 0, y: -10, scale: 0.8 }}
            className="absolute -top-16 z-10 max-w-[200px] rounded-2xl bg-white px-4 py-2 text-center text-sm font-body text-gray-700 shadow-lg"
          >
            {message}
            <div className="absolute -bottom-2 left-1/2 h-4 w-4 -translate-x-1/2 rotate-45 bg-white" />
          </motion.div>
        )}
      </AnimatePresence>

      {/* Creature SVG */}
      <motion.div
        className="cursor-pointer select-none"
        animate={getAnimation(state.mood)}
        whileHover={{ scale: 1.1 }}
        whileTap={{ scale: 0.95 }}
      >
        <svg
          width={px}
          height={px}
          viewBox="0 0 200 200"
          fill="none"
          xmlns="http://www.w3.org/2000/svg"
        >
          {/* Background glow */}
          <motion.circle
            cx="100"
            cy="110"
            r="70"
            fill={color}
            opacity="0.15"
            animate={{ r: [70, 75, 70], opacity: [0.15, 0.25, 0.15] }}
            transition={{ duration: 3, repeat: Infinity, ease: "easeInOut" }}
          />

          {/* Body */}
          <CreatureBody mood={state.mood} color={color} />

          {/* Face */}
          <CreatureFace mood={state.mood} />

          {/* Accessories based on mood */}
          <CreatureAccessories mood={state.mood} />
        </svg>
      </motion.div>

      {/* Happiness bar */}
      <div className="mt-2 w-24 overflow-hidden rounded-full bg-gray-200/50">
        <motion.div
          className="h-2 rounded-full"
          style={{ backgroundColor: color }}
          initial={{ width: "0%" }}
          animate={{ width: `${state.happiness}%` }}
          transition={{ duration: 0.8, ease: "easeOut" }}
        />
      </div>

      {/* Level badge */}
      <div className="mt-1 flex items-center gap-1">
        <span className="text-xs font-body text-gray-400">Lv.{state.level}</span>
        {state.streak > 0 && (
          <span className="text-xs font-body text-orange-400">
            🔥{state.streak}
          </span>
        )}
      </div>
    </div>
  );
}

function CreatureBody({ mood, color }: { mood: CreatureMood; color: string }) {
  const isHappy = ["ecstatic", "happy", "content"].includes(mood);
  const isSad = ["sad", "crying"].includes(mood);

  return (
    <g>
      {/* Main blob body */}
      <motion.ellipse
        cx="100"
        cy="115"
        rx={isHappy ? 55 : isSad ? 48 : 50}
        ry={isHappy ? 50 : isSad ? 52 : 50}
        fill={color}
        animate={
          isSad
            ? { ry: [52, 50, 52], rx: [48, 46, 48] }
            : { ry: [50, 48, 50], rx: [55, 53, 55] }
        }
        transition={{ duration: 2, repeat: Infinity, ease: "easeInOut" }}
      />

      {/* Belly highlight */}
      <ellipse cx="100" cy="120" rx="30" ry="25" fill="white" opacity="0.3" />

      {/* Cheeks */}
      {isHappy && (
        <>
          <motion.circle
            cx="72"
            cy="118"
            r="8"
            fill="#FFB6C1"
            opacity="0.6"
            animate={{ opacity: [0.4, 0.7, 0.4] }}
            transition={{ duration: 2, repeat: Infinity }}
          />
          <motion.circle
            cx="128"
            cy="118"
            r="8"
            fill="#FFB6C1"
            opacity="0.6"
            animate={{ opacity: [0.4, 0.7, 0.4] }}
            transition={{ duration: 2, repeat: Infinity, delay: 0.5 }}
          />
        </>
      )}

      {/* Ears/nubs */}
      <ellipse cx="65" cy="80" rx="12" ry="16" fill={color} />
      <ellipse cx="135" cy="80" rx="12" ry="16" fill={color} />
      <ellipse cx="65" cy="80" rx="7" ry="10" fill="white" opacity="0.3" />
      <ellipse cx="135" cy="80" rx="7" ry="10" fill="white" opacity="0.3" />

      {/* Little feet */}
      <ellipse cx="80" cy="160" rx="14" ry="7" fill={color} />
      <ellipse cx="120" cy="160" rx="14" ry="7" fill={color} />
    </g>
  );
}

function CreatureFace({ mood }: { mood: CreatureMood }) {
  return (
    <g>
      {/* Eyes */}
      <CreatureEyes mood={mood} />

      {/* Mouth */}
      <CreatureMouth mood={mood} />
    </g>
  );
}

function CreatureEyes({ mood }: { mood: CreatureMood }) {
  switch (mood) {
    case "ecstatic":
      return (
        <>
          {/* Sparkle eyes */}
          <motion.g
            animate={{ scale: [1, 1.1, 1] }}
            transition={{ duration: 0.8, repeat: Infinity }}
          >
            <path d="M82 105 L86 100 L90 105 L86 110 Z" fill="#333" />
            <circle cx="86" cy="103" r="1.5" fill="white" />
          </motion.g>
          <motion.g
            animate={{ scale: [1, 1.1, 1] }}
            transition={{ duration: 0.8, repeat: Infinity, delay: 0.2 }}
          >
            <path d="M110 105 L114 100 L118 105 L114 110 Z" fill="#333" />
            <circle cx="114" cy="103" r="1.5" fill="white" />
          </motion.g>
        </>
      );

    case "happy":
      return (
        <>
          {/* Happy closed/squint eyes */}
          <path
            d="M80 106 Q86 100 92 106"
            stroke="#333"
            strokeWidth="3"
            strokeLinecap="round"
            fill="none"
          />
          <path
            d="M108 106 Q114 100 120 106"
            stroke="#333"
            strokeWidth="3"
            strokeLinecap="round"
            fill="none"
          />
        </>
      );

    case "content":
      return (
        <>
          <circle cx="86" cy="105" r="5" fill="#333" />
          <circle cx="114" cy="105" r="5" fill="#333" />
          <circle cx="88" cy="103" r="2" fill="white" />
          <circle cx="116" cy="103" r="2" fill="white" />
        </>
      );

    case "neutral":
      return (
        <>
          <circle cx="86" cy="106" r="4.5" fill="#555" />
          <circle cx="114" cy="106" r="4.5" fill="#555" />
          <circle cx="87.5" cy="104.5" r="1.5" fill="white" />
          <circle cx="115.5" cy="104.5" r="1.5" fill="white" />
        </>
      );

    case "lonely":
      return (
        <>
          {/* Slightly droopy eyes */}
          <ellipse cx="86" cy="108" rx="5" ry="4" fill="#555" />
          <ellipse cx="114" cy="108" rx="5" ry="4" fill="#555" />
          <circle cx="87" cy="107" r="1.5" fill="white" />
          <circle cx="115" cy="107" r="1.5" fill="white" />
          {/* Slight eyebrow droop */}
          <path
            d="M78 98 Q86 96 92 99"
            stroke="#777"
            strokeWidth="1.5"
            fill="none"
          />
          <path
            d="M108 99 Q114 96 122 98"
            stroke="#777"
            strokeWidth="1.5"
            fill="none"
          />
        </>
      );

    case "sad":
      return (
        <>
          {/* Big sad eyes */}
          <ellipse cx="86" cy="108" rx="6" ry="7" fill="#555" />
          <ellipse cx="114" cy="108" rx="6" ry="7" fill="#555" />
          <circle cx="88" cy="106" r="2.5" fill="white" />
          <circle cx="116" cy="106" r="2.5" fill="white" />
          {/* Sad eyebrows */}
          <path
            d="M76 96 Q86 100 94 98"
            stroke="#777"
            strokeWidth="2"
            fill="none"
          />
          <path
            d="M106 98 Q114 100 124 96"
            stroke="#777"
            strokeWidth="2"
            fill="none"
          />
        </>
      );

    case "crying":
      return (
        <>
          {/* Watery eyes */}
          <ellipse cx="86" cy="108" rx="7" ry="8" fill="#555" />
          <ellipse cx="114" cy="108" rx="7" ry="8" fill="#555" />
          <circle cx="89" cy="105" r="3" fill="white" />
          <circle cx="117" cy="105" r="3" fill="white" />
          {/* Sad eyebrows */}
          <path
            d="M74 94 Q86 100 94 97"
            stroke="#777"
            strokeWidth="2"
            fill="none"
          />
          <path
            d="M106 97 Q114 100 126 94"
            stroke="#777"
            strokeWidth="2"
            fill="none"
          />
          {/* Tears */}
          <motion.ellipse
            cx="78"
            cy="118"
            rx="2"
            ry="4"
            fill="#87CEEB"
            opacity="0.8"
            animate={{ cy: [118, 140], opacity: [0.8, 0] }}
            transition={{ duration: 1.5, repeat: Infinity, ease: "easeIn" }}
          />
          <motion.ellipse
            cx="122"
            cy="118"
            rx="2"
            ry="4"
            fill="#87CEEB"
            opacity="0.8"
            animate={{ cy: [118, 140], opacity: [0.8, 0] }}
            transition={{
              duration: 1.5,
              repeat: Infinity,
              ease: "easeIn",
              delay: 0.7,
            }}
          />
        </>
      );
  }
}

function CreatureMouth({ mood }: { mood: CreatureMood }) {
  switch (mood) {
    case "ecstatic":
      return (
        <motion.path
          d="M88 122 Q100 136 112 122"
          stroke="#333"
          strokeWidth="2.5"
          strokeLinecap="round"
          fill="#FF85A1"
          fillOpacity="0.3"
          animate={{ d: ["M88 122 Q100 136 112 122", "M88 123 Q100 138 112 123", "M88 122 Q100 136 112 122"] }}
          transition={{ duration: 1, repeat: Infinity }}
        />
      );

    case "happy":
      return (
        <path
          d="M90 122 Q100 132 110 122"
          stroke="#333"
          strokeWidth="2"
          strokeLinecap="round"
          fill="none"
        />
      );

    case "content":
      return (
        <path
          d="M92 122 Q100 128 108 122"
          stroke="#333"
          strokeWidth="2"
          strokeLinecap="round"
          fill="none"
        />
      );

    case "neutral":
      return (
        <line
          x1="92"
          y1="124"
          x2="108"
          y2="124"
          stroke="#555"
          strokeWidth="2"
          strokeLinecap="round"
        />
      );

    case "lonely":
      return (
        <path
          d="M92 126 Q100 122 108 126"
          stroke="#555"
          strokeWidth="2"
          strokeLinecap="round"
          fill="none"
        />
      );

    case "sad":
      return (
        <path
          d="M90 128 Q100 120 110 128"
          stroke="#555"
          strokeWidth="2"
          strokeLinecap="round"
          fill="none"
        />
      );

    case "crying":
      return (
        <motion.path
          d="M88 130 Q100 118 112 130"
          stroke="#555"
          strokeWidth="2.5"
          strokeLinecap="round"
          fill="none"
          animate={{
            d: [
              "M88 130 Q100 118 112 130",
              "M88 129 Q100 120 112 129",
              "M88 130 Q100 118 112 130",
            ],
          }}
          transition={{ duration: 2, repeat: Infinity }}
        />
      );
  }
}

function CreatureAccessories({ mood }: { mood: CreatureMood }) {
  if (mood === "ecstatic") {
    // Sparkles around the creature
    return (
      <g>
        {[
          { x: 50, y: 70, delay: 0 },
          { x: 150, y: 75, delay: 0.5 },
          { x: 45, y: 130, delay: 1 },
          { x: 155, y: 125, delay: 0.3 },
          { x: 100, y: 55, delay: 0.7 },
        ].map((star, i) => (
          <motion.g
            key={i}
            animate={{ opacity: [0, 1, 0], scale: [0, 1, 0] }}
            transition={{
              duration: 1.5,
              repeat: Infinity,
              delay: star.delay,
            }}
          >
            <text
              x={star.x}
              y={star.y}
              fontSize="12"
              textAnchor="middle"
              fill="#FFD93D"
            >
              ✦
            </text>
          </motion.g>
        ))}
      </g>
    );
  }

  if (mood === "happy") {
    // Little music notes
    return (
      <g>
        <motion.text
          x="145"
          y="80"
          fontSize="14"
          fill="#A8E6CF"
          animate={{ y: [80, 65], opacity: [1, 0] }}
          transition={{ duration: 2, repeat: Infinity }}
        >
          ♪
        </motion.text>
        <motion.text
          x="155"
          y="90"
          fontSize="10"
          fill="#A8E6CF"
          animate={{ y: [90, 72], opacity: [1, 0] }}
          transition={{ duration: 2, repeat: Infinity, delay: 0.8 }}
        >
          ♫
        </motion.text>
      </g>
    );
  }

  if (mood === "crying") {
    // Rain cloud
    return (
      <g opacity="0.4">
        <ellipse cx="100" cy="52" rx="25" ry="12" fill="#9DB2CE" />
        <ellipse cx="85" cy="50" rx="12" ry="10" fill="#9DB2CE" />
        <ellipse cx="115" cy="50" rx="12" ry="10" fill="#9DB2CE" />
        {[85, 100, 115].map((x, i) => (
          <motion.line
            key={i}
            x1={x}
            y1="62"
            x2={x - 2}
            y2="72"
            stroke="#87CEEB"
            strokeWidth="1.5"
            opacity="0.5"
            animate={{ y1: [62, 64], y2: [72, 74], opacity: [0.5, 0] }}
            transition={{
              duration: 1,
              repeat: Infinity,
              delay: i * 0.3,
            }}
          />
        ))}
      </g>
    );
  }

  return null;
}

function getAnimation(mood: CreatureMood) {
  switch (mood) {
    case "ecstatic":
      return {
        y: [0, -8, 0],
        rotate: [-2, 2, -2],
        transition: { duration: 1.2, repeat: Infinity, ease: "easeInOut" },
      };
    case "happy":
      return {
        y: [0, -5, 0],
        transition: { duration: 2, repeat: Infinity, ease: "easeInOut" },
      };
    case "content":
      return {
        y: [0, -3, 0],
        transition: { duration: 3, repeat: Infinity, ease: "easeInOut" },
      };
    case "neutral":
      return {
        y: [0, -1, 0],
        transition: { duration: 4, repeat: Infinity, ease: "easeInOut" },
      };
    case "lonely":
      return {
        rotate: [-1, 1, -1],
        transition: { duration: 4, repeat: Infinity, ease: "easeInOut" },
      };
    case "sad":
      return {
        y: [0, 2, 0],
        transition: { duration: 3, repeat: Infinity, ease: "easeInOut" },
      };
    case "crying":
      return {
        y: [0, 2, 0],
        rotate: [-1, 1, -1],
        transition: { duration: 2.5, repeat: Infinity, ease: "easeInOut" },
      };
  }
}
