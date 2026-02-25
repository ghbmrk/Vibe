import { NextRequest, NextResponse } from "next/server";

/**
 * API route to track and retrieve chat activity.
 * In a full implementation, this would query the Telegram Bot API
 * for actual message history and counts.
 *
 * For now it serves as a placeholder that the frontend can call
 * to sync activity data.
 */
export async function GET(request: NextRequest) {
  const chatId = request.nextUrl.searchParams.get("chatId");

  if (!chatId) {
    return NextResponse.json(
      { ok: false, error: "chatId is required" },
      { status: 400 }
    );
  }

  // In a real implementation, fetch from Telegram Bot API:
  // const botToken = process.env.TELEGRAM_BOT_TOKEN;
  // const updates = await fetch(`https://api.telegram.org/bot${botToken}/getUpdates`);

  return NextResponse.json({
    ok: true,
    activity: {
      chatId: Number(chatId),
      messageCount: 0,
      lastMessageAt: Date.now(),
      dailyCounts: {},
    },
  });
}

export async function POST(request: NextRequest) {
  try {
    const body = await request.json();
    const { chatId, messageCount } = body;

    if (!chatId) {
      return NextResponse.json(
        { ok: false, error: "chatId is required" },
        { status: 400 }
      );
    }

    // In a full implementation, this would store activity data
    // in a database and sync with Telegram.

    return NextResponse.json({
      ok: true,
      recorded: {
        chatId,
        messageCount: messageCount || 1,
        timestamp: Date.now(),
      },
    });
  } catch {
    return NextResponse.json(
      { ok: false, error: "Invalid request" },
      { status: 400 }
    );
  }
}
