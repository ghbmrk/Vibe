"use client";

import { useState, useEffect, useCallback } from "react";
import { TelegramUser, TelegramChat } from "@/types";
import {
  saveUser,
  getUser,
  saveSelectedChat,
  getSelectedChat,
  isOnboarded,
  clearAll,
} from "@/lib/storage";

type AuthStep = "login" | "select-chat" | "ready";

export function useTelegramAuth() {
  const [user, setUser] = useState<TelegramUser | null>(null);
  const [selectedChat, setSelectedChat] = useState<TelegramChat | null>(null);
  const [step, setStep] = useState<AuthStep>("login");

  // Load saved auth state on mount
  useEffect(() => {
    const savedUser = getUser();
    const savedChat = getSelectedChat();
    const onboarded = isOnboarded();

    if (savedUser) {
      setUser(savedUser);
      if (onboarded && savedChat) {
        setSelectedChat(savedChat);
        setStep("ready");
      } else {
        setStep("select-chat");
      }
    }
  }, []);

  const login = useCallback((telegramUser: TelegramUser) => {
    setUser(telegramUser);
    saveUser(telegramUser);
    setStep("select-chat");
  }, []);

  const selectChat = useCallback((chat: TelegramChat) => {
    setSelectedChat(chat);
    saveSelectedChat(chat);
    setStep("ready");
  }, []);

  const logout = useCallback(() => {
    clearAll();
    setUser(null);
    setSelectedChat(null);
    setStep("login");
  }, []);

  return { user, selectedChat, step, login, selectChat, logout };
}
