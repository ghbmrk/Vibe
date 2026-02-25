"use client";

import { motion, AnimatePresence } from "framer-motion";
import Header from "@/components/Header";
import TelegramLoginButton from "@/components/TelegramLoginButton";
import ChatSelector from "@/components/ChatSelector";
import TamagotchiCreature from "@/components/TamagotchiCreature";
import MoodIndicator from "@/components/MoodIndicator";
import StatsPanel from "@/components/StatsPanel";
import ChatBubble from "@/components/ChatBubble";
import { useTelegramAuth } from "@/hooks/useTelegramAuth";
import { useCreatureMood } from "@/hooks/useCreatureMood";
import { useState, useRef, useEffect } from "react";

const BOT_USERNAME = process.env.NEXT_PUBLIC_TELEGRAM_BOT_USERNAME || "";

export default function Home() {
  const { user, selectedChat, step, login, selectChat, logout } =
    useTelegramAuth();
  const { creature, recordMessage } = useCreatureMood();
  const [messages, setMessages] = useState<
    { text: string; isBot: boolean; time: string }[]
  >([]);
  const [inputValue, setInputValue] = useState("");
  const chatEndRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    chatEndRef.current?.scrollIntoView({ behavior: "smooth" });
  }, [messages]);

  const handleSendMessage = () => {
    const text = inputValue.trim();
    if (!text) return;

    const now = new Date().toLocaleTimeString([], {
      hour: "2-digit",
      minute: "2-digit",
    });

    // Add user message
    setMessages((prev) => [...prev, { text, isBot: false, time: now }]);
    setInputValue("");
    recordMessage(1);

    // Simulate bot response after a delay
    setTimeout(() => {
      const response = getBotResponse(text, creature.mood);
      setMessages((prev) => [
        ...prev,
        {
          text: response,
          isBot: true,
          time: new Date().toLocaleTimeString([], {
            hour: "2-digit",
            minute: "2-digit",
          }),
        },
      ]);
      recordMessage(1);
    }, 800 + Math.random() * 1200);
  };

  return (
    <div className="flex min-h-screen flex-col">
      <Header user={user} onLogout={step === "ready" ? logout : undefined} />

      <main className="flex flex-1 flex-col items-center px-4 pb-4">
        <AnimatePresence mode="wait">
          {/* Step 1: Login */}
          {step === "login" && (
            <motion.div
              key="login"
              initial={{ opacity: 0, y: 20 }}
              animate={{ opacity: 1, y: 0 }}
              exit={{ opacity: 0, y: -20 }}
              className="flex flex-1 flex-col items-center justify-center gap-8"
            >
              {/* Hero creature */}
              <TamagotchiCreature
                state={{ ...creature, mood: "happy", happiness: 80 }}
                size="lg"
              />

              <div className="text-center">
                <h1 className="font-display text-3xl font-extrabold text-gray-800">
                  Meet your <span className="text-gradient">Vibe</span> buddy!
                </h1>
                <p className="mt-2 max-w-xs font-body text-gray-500">
                  A cute companion that lives in your Telegram chats.
                  Keep chatting to keep it happy!
                </p>
              </div>

              <TelegramLoginButton
                botUsername={BOT_USERNAME}
                onAuth={login}
                useDemoMode={!BOT_USERNAME}
              />

              {/* Feature highlights */}
              <div className="grid grid-cols-3 gap-4 pt-4">
                {[
                  { icon: "💬", label: "Chat Together" },
                  { icon: "📈", label: "Level Up" },
                  { icon: "💕", label: "Stay Happy" },
                ].map((feat) => (
                  <div
                    key={feat.label}
                    className="flex flex-col items-center gap-1 text-center"
                  >
                    <span className="text-2xl">{feat.icon}</span>
                    <span className="text-xs font-body text-gray-500">
                      {feat.label}
                    </span>
                  </div>
                ))}
              </div>
            </motion.div>
          )}

          {/* Step 2: Select Chat */}
          {step === "select-chat" && (
            <motion.div
              key="onboarding"
              initial={{ opacity: 0, y: 20 }}
              animate={{ opacity: 1, y: 0 }}
              exit={{ opacity: 0, y: -20 }}
              className="flex flex-1 flex-col items-center justify-center gap-6"
            >
              <TamagotchiCreature state={creature} size="sm" />
              <ChatSelector onSelect={selectChat} />
            </motion.div>
          )}

          {/* Step 3: Dashboard */}
          {step === "ready" && selectedChat && (
            <motion.div
              key="dashboard"
              initial={{ opacity: 0 }}
              animate={{ opacity: 1 }}
              className="flex w-full flex-1 flex-col gap-4"
            >
              {/* Creature + mood area */}
              <div className="flex flex-col items-center gap-3 pt-2">
                <TamagotchiCreature state={creature} size="md" />
                <MoodIndicator
                  mood={creature.mood}
                  happiness={creature.happiness}
                />
              </div>

              {/* Stats */}
              <StatsPanel creature={creature} />

              {/* Chat area */}
              <div className="glass-card flex flex-1 flex-col overflow-hidden">
                {/* Chat header */}
                <div className="flex items-center gap-3 border-b border-gray-100 px-4 py-3">
                  <div className="flex h-8 w-8 items-center justify-center rounded-full bg-gradient-to-br from-lavender to-bubblegum text-sm">
                    🦀
                  </div>
                  <div>
                    <h3 className="font-display text-sm font-bold text-gray-700">
                      {selectedChat.title}
                    </h3>
                    <p className="text-[10px] font-body text-gray-400">
                      @{selectedChat.bot_username}
                    </p>
                  </div>
                </div>

                {/* Messages */}
                <div className="flex-1 space-y-3 overflow-y-auto p-4">
                  {messages.length === 0 && (
                    <div className="flex h-full flex-col items-center justify-center gap-2 py-8 text-center">
                      <span className="text-4xl">👋</span>
                      <p className="font-body text-sm text-gray-400">
                        Say hello to keep your buddy happy!
                      </p>
                    </div>
                  )}
                  {messages.map((msg, i) => (
                    <ChatBubble
                      key={i}
                      message={msg.text}
                      isBot={msg.isBot}
                      timestamp={msg.time}
                    />
                  ))}
                  <div ref={chatEndRef} />
                </div>

                {/* Input */}
                <div className="safe-bottom border-t border-gray-100 p-3">
                  <div className="flex items-center gap-2">
                    <input
                      type="text"
                      value={inputValue}
                      onChange={(e) => setInputValue(e.target.value)}
                      onKeyDown={(e) => {
                        if (e.key === "Enter" && !e.shiftKey) {
                          e.preventDefault();
                          handleSendMessage();
                        }
                      }}
                      placeholder="Type a message..."
                      className="chat-input flex-1 rounded-2xl bg-gray-50 px-4 py-2.5 font-body text-sm text-gray-700 placeholder-gray-300"
                    />
                    <motion.button
                      onClick={handleSendMessage}
                      className="flex h-10 w-10 items-center justify-center rounded-full bg-lavender text-white shadow-sm"
                      whileHover={{ scale: 1.1 }}
                      whileTap={{ scale: 0.9 }}
                      disabled={!inputValue.trim()}
                    >
                      <svg
                        width="18"
                        height="18"
                        viewBox="0 0 24 24"
                        fill="currentColor"
                      >
                        <path d="M2.01 21L23 12 2.01 3 2 10l15 2-15 2z" />
                      </svg>
                    </motion.button>
                  </div>
                </div>
              </div>
            </motion.div>
          )}
        </AnimatePresence>
      </main>
    </div>
  );
}

function getBotResponse(userMessage: string, _mood: string): string {
  const lower = userMessage.toLowerCase();
  const responses: [RegExp, string[]][] = [
    [/^(hi|hello|hey|sup|yo)/i, [
      "Hey there! How's your day going? 😊",
      "Hello friend! What's on your mind?",
      "Hi! I was hoping you'd stop by!",
    ]],
    [/how are you/i, [
      "I'm doing great now that you're here!",
      "Better now that we're chatting! How about you?",
      "I'm wonderful, thanks for asking! 💜",
    ]],
    [/thank/i, [
      "You're welcome! Always happy to help!",
      "Anytime! That's what I'm here for 😊",
      "No problem at all!",
    ]],
    [/(bye|goodbye|see you|gotta go)/i, [
      "Come back soon! I'll miss you! 💕",
      "Bye for now! Don't forget about me~",
      "See you later! Stay awesome!",
    ]],
    [/(help|what can you do)/i, [
      "I'm your chat buddy! Just talk to me and watch your creature grow happier! 🌟",
      "I'm here to chat! The more we talk, the happier your Vibe buddy gets!",
    ]],
  ];

  for (const [pattern, replies] of responses) {
    if (pattern.test(lower)) {
      return replies[Math.floor(Math.random() * replies.length)];
    }
  }

  const generic = [
    "That's interesting! Tell me more~",
    "Oh really? I love hearing about that!",
    "Hmm, that's cool! What else is going on?",
    "Nice! Keep the good vibes going! ✨",
    "I'm listening! Go on~ 😊",
    "That sounds fun! What happened next?",
    "Love it! You always have the best stories!",
  ];
  return generic[Math.floor(Math.random() * generic.length)];
}
