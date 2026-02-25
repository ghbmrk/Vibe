export interface TelegramUser {
  id: number;
  first_name: string;
  last_name?: string;
  username?: string;
  photo_url?: string;
  auth_date: number;
  hash: string;
}

export interface TelegramChat {
  id: number;
  title: string;
  type: "private" | "group" | "supergroup" | "channel";
  photo_url?: string;
  last_message_date?: number;
  bot_username?: string;
}

export type CreatureMood =
  | "ecstatic"
  | "happy"
  | "content"
  | "neutral"
  | "lonely"
  | "sad"
  | "crying";

export interface CreatureState {
  mood: CreatureMood;
  level: number;
  happiness: number; // 0-100
  lastInteraction: number; // timestamp
  totalMessages: number;
  streak: number; // consecutive days of chatting
}

export interface ChatActivity {
  chatId: number;
  messageCount: number;
  lastMessageAt: number;
  dailyCounts: Record<string, number>; // date string -> count
}

export interface AppState {
  user: TelegramUser | null;
  selectedChat: TelegramChat | null;
  creature: CreatureState;
  activity: ChatActivity | null;
  onboarded: boolean;
}

export const MOOD_THRESHOLDS: Record<CreatureMood, number> = {
  ecstatic: 90,
  happy: 70,
  content: 50,
  neutral: 35,
  lonely: 20,
  sad: 10,
  crying: 0,
};

export const MOOD_COLORS: Record<CreatureMood, string> = {
  ecstatic: "#FFD93D",
  happy: "#A8E6CF",
  content: "#87CEEB",
  neutral: "#C3A6FF",
  lonely: "#FFB6C1",
  sad: "#9DB2CE",
  crying: "#7A8FA6",
};
