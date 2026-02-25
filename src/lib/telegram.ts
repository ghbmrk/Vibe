import crypto from "crypto";
import { TelegramUser } from "@/types";

/**
 * Verify Telegram Login Widget authentication data.
 * See: https://core.telegram.org/widgets/login#checking-authorization
 */
export function verifyTelegramAuth(
  data: TelegramUser,
  botToken: string
): boolean {
  const { hash, ...rest } = data;

  // Create data-check-string
  const checkString = Object.keys(rest)
    .sort()
    .map((key) => `${key}=${rest[key as keyof typeof rest]}`)
    .join("\n");

  // Create secret key from bot token
  const secretKey = crypto.createHash("sha256").update(botToken).digest();

  // Calculate HMAC
  const hmac = crypto
    .createHmac("sha256", secretKey)
    .update(checkString)
    .digest("hex");

  return hmac === hash;
}

/**
 * Check if auth data is not too old (within 24 hours).
 */
export function isAuthFresh(authDate: number): boolean {
  const now = Math.floor(Date.now() / 1000);
  return now - authDate < 86400;
}
