import { AppState, CreatureState, TelegramChat, TelegramUser, ChatActivity } from "@/types";
import { createInitialCreatureState } from "./mood";

const STORAGE_KEY = "vibe_tamagotchi";

function getStorage(): AppState | null {
  if (typeof window === "undefined") return null;
  try {
    const data = localStorage.getItem(STORAGE_KEY);
    return data ? JSON.parse(data) : null;
  } catch {
    return null;
  }
}

function setStorage(state: Partial<AppState>): void {
  if (typeof window === "undefined") return;
  const current = getStorage() || getDefaultState();
  const updated = { ...current, ...state };
  localStorage.setItem(STORAGE_KEY, JSON.stringify(updated));
}

export function getDefaultState(): AppState {
  return {
    user: null,
    selectedChat: null,
    creature: createInitialCreatureState(),
    activity: null,
    onboarded: false,
  };
}

export function saveUser(user: TelegramUser): void {
  setStorage({ user });
}

export function getUser(): TelegramUser | null {
  return getStorage()?.user ?? null;
}

export function saveSelectedChat(chat: TelegramChat): void {
  setStorage({ selectedChat: chat, onboarded: true });
}

export function getSelectedChat(): TelegramChat | null {
  return getStorage()?.selectedChat ?? null;
}

export function saveCreatureState(creature: CreatureState): void {
  setStorage({ creature });
}

export function getCreatureState(): CreatureState {
  return getStorage()?.creature ?? createInitialCreatureState();
}

export function saveActivity(activity: ChatActivity): void {
  setStorage({ activity });
}

export function getActivity(): ChatActivity | null {
  return getStorage()?.activity ?? null;
}

export function isOnboarded(): boolean {
  return getStorage()?.onboarded ?? false;
}

export function getAppState(): AppState {
  return getStorage() ?? getDefaultState();
}

export function clearAll(): void {
  if (typeof window === "undefined") return;
  localStorage.removeItem(STORAGE_KEY);
}
