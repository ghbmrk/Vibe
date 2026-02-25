import { CreatureMood, CreatureState, MOOD_THRESHOLDS } from "@/types";

const HOUR_MS = 60 * 60 * 1000;
const DAY_MS = 24 * HOUR_MS;

/**
 * Calculate happiness score (0-100) based on chat activity patterns.
 * Happiness decays over time without interaction and increases with messages.
 */
export function calculateHappiness(
  lastInteraction: number,
  totalMessages: number,
  streak: number,
  now: number = Date.now()
): number {
  const timeSinceLast = now - lastInteraction;

  // Base happiness from recency of last interaction
  let recencyScore: number;
  if (timeSinceLast < HOUR_MS) {
    recencyScore = 100;
  } else if (timeSinceLast < 3 * HOUR_MS) {
    recencyScore = 85;
  } else if (timeSinceLast < 6 * HOUR_MS) {
    recencyScore = 70;
  } else if (timeSinceLast < 12 * HOUR_MS) {
    recencyScore = 55;
  } else if (timeSinceLast < DAY_MS) {
    recencyScore = 40;
  } else if (timeSinceLast < 2 * DAY_MS) {
    recencyScore = 25;
  } else if (timeSinceLast < 3 * DAY_MS) {
    recencyScore = 15;
  } else {
    recencyScore = Math.max(0, 10 - Math.floor(timeSinceLast / DAY_MS));
  }

  // Bonus from streak (up to +15)
  const streakBonus = Math.min(streak * 3, 15);

  // Bonus from total engagement (up to +10)
  const engagementBonus = Math.min(Math.floor(totalMessages / 10), 10);

  return Math.min(100, Math.max(0, recencyScore + streakBonus + engagementBonus));
}

/**
 * Determine creature mood from happiness score.
 */
export function getMoodFromHappiness(happiness: number): CreatureMood {
  const moods: CreatureMood[] = [
    "ecstatic",
    "happy",
    "content",
    "neutral",
    "lonely",
    "sad",
    "crying",
  ];

  for (const mood of moods) {
    if (happiness >= MOOD_THRESHOLDS[mood]) {
      return mood;
    }
  }
  return "crying";
}

/**
 * Get a status message based on the creature's mood.
 */
export function getMoodMessage(mood: CreatureMood): string {
  const messages: Record<CreatureMood, string[]> = {
    ecstatic: [
      "I'm SO happy to see you!! 💕",
      "You're the best friend ever!",
      "I love chatting with you! ✨",
      "This is the best day ever!!",
    ],
    happy: [
      "Yay, you're here! 🌟",
      "I'm having a great day!",
      "Let's keep chatting!",
      "You make me so happy~",
    ],
    content: [
      "It's nice to see you!",
      "I'm doing well today~",
      "Thanks for stopping by!",
      "Everything feels good ☺",
    ],
    neutral: [
      "Oh, hi there...",
      "It's been a little while.",
      "I was just thinking of you.",
      "Good to see a friendly face.",
    ],
    lonely: [
      "I've been waiting for you...",
      "It gets quiet here alone.",
      "I missed you a little...",
      "Will you stay a while?",
    ],
    sad: [
      "You came back... 🥺",
      "I was starting to worry...",
      "It's been so long...",
      "Please don't leave again...",
    ],
    crying: [
      "I thought you forgot me... 😢",
      "Please talk to me...",
      "I've been so alone...",
      "I really missed you...",
    ],
  };

  const options = messages[mood];
  return options[Math.floor(Math.random() * options.length)];
}

/**
 * Calculate the level based on total messages.
 */
export function calculateLevel(totalMessages: number): number {
  return Math.floor(Math.sqrt(totalMessages / 5)) + 1;
}

/**
 * Create an updated creature state based on current activity.
 */
export function updateCreatureState(
  current: CreatureState,
  newMessageCount?: number
): CreatureState {
  const now = Date.now();
  const totalMessages = current.totalMessages + (newMessageCount ?? 0);
  const lastInteraction = newMessageCount ? now : current.lastInteraction;

  // Update streak
  const lastDate = new Date(current.lastInteraction).toDateString();
  const today = new Date(now).toDateString();
  const yesterday = new Date(now - DAY_MS).toDateString();

  let streak = current.streak;
  if (newMessageCount) {
    if (lastDate === yesterday && today !== lastDate) {
      streak += 1;
    } else if (lastDate !== today && lastDate !== yesterday) {
      streak = 1;
    }
  }

  const happiness = calculateHappiness(lastInteraction, totalMessages, streak, now);
  const mood = getMoodFromHappiness(happiness);
  const level = calculateLevel(totalMessages);

  return { mood, level, happiness, lastInteraction, totalMessages, streak };
}

/**
 * Create an initial creature state.
 */
export function createInitialCreatureState(): CreatureState {
  return {
    mood: "content",
    level: 1,
    happiness: 60,
    lastInteraction: Date.now(),
    totalMessages: 0,
    streak: 0,
  };
}
