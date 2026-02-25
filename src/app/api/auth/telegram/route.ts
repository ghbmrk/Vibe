import { NextRequest, NextResponse } from "next/server";
import { verifyTelegramAuth, isAuthFresh } from "@/lib/telegram";
import { TelegramUser } from "@/types";

export async function POST(request: NextRequest) {
  try {
    const body: TelegramUser = await request.json();
    const botToken = process.env.TELEGRAM_BOT_TOKEN;

    // In demo mode (no bot token), accept any auth data
    if (!botToken) {
      return NextResponse.json({
        ok: true,
        user: body,
        demo: true,
      });
    }

    // Verify auth is not stale
    if (!isAuthFresh(body.auth_date)) {
      return NextResponse.json(
        { ok: false, error: "Auth data is too old" },
        { status: 401 }
      );
    }

    // Verify Telegram hash
    if (!verifyTelegramAuth(body, botToken)) {
      return NextResponse.json(
        { ok: false, error: "Invalid auth data" },
        { status: 401 }
      );
    }

    return NextResponse.json({ ok: true, user: body });
  } catch {
    return NextResponse.json(
      { ok: false, error: "Invalid request" },
      { status: 400 }
    );
  }
}
