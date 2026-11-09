import type { Metadata } from "next";
import "./globals.css";

export const metadata: Metadata = {
  title: "BeatBlocks — Build a beat together",
  description: "Place blocks, make a beat, and invite a friend to build it with you.",
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en">
      <body className="antialiased">{children}</body>
    </html>
  );
}
