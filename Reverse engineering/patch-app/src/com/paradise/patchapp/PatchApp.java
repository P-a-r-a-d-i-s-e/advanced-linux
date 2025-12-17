package com.paradise.patchapp;

import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;

public class PatchApp {

    public static void main(String[] args) throws IOException {
        String originalFilePath = "D:/Projects/Advanced Linux/Reverse engineering/patch-app/resources/hack_app";
        String backupFilePath = originalFilePath + "_patch";

        Files.copy(new File(originalFilePath).toPath(), new File(backupFilePath).toPath(), StandardCopyOption.REPLACE_EXISTING);
        System.out.println("Backup created: " + backupFilePath);

        try (RandomAccessFile raf = new RandomAccessFile(backupFilePath, "rw")) {
            raf.seek(5175); // отступаем offset
            byte[] bytesToReplace = new byte[7]; // читаем только 7 байт
            raf.read(bytesToReplace);

            if (bytesToReplace[3] == 0x00 && bytesToReplace[4] == 0x00 && bytesToReplace[5] == 0x00 && bytesToReplace[6] == 0x00) {
                System.out.println("Pattern found, replacing...");
                raf.seek(5175 + 3); // начинаем с 3-го байта, т.к. в начале идут c7 45 e4
                raf.write(new byte[]{0x01, 0x00, 0x00, 0x00}); // заменяем на 01 00 00 00
                System.out.println("Replacement done.");
            } else {
                System.out.println("No pattern found at this offset.");
            }
        }
    }
}
