🛠️ ESP32 & FreeRTOS: Tasarladığım Çok Fonksiyonlu Atölye Yardımcısı
🎯 Projenin Hikayesi ve Amacım
Atölye çalışmalarım sırasında ortam koşullarını (sıcaklık, ışık, gürültü) anlık olarak takip edebileceğim, kompakt ve güvenilir bir yardımcıya ihtiyaç duydum. Piyasadaki tekil çözümler yerine, "Dijital Bir İsviçre Çakısı" vizyonuyla, tüm bu verileri tek bir ekranda toplayan bu cihazı tasarladım .

Bu projeyi geliştirirken asıl hedefim, sadece sensör verisi okumak değil; gömülü sistemlerde sıkça karşılaşılan performans darboğazlarını modern yöntemlerle aşmaktı.

💡 Teknik Yaklaşımım: Neden FreeRTOS Seçtim?
Geleneksel Arduino projelerinde kullanılan loop() (tek döngü) mimarisinin, özellikle DHT11 gibi yavaş sensörleri okurken sistemi blokladığını (dondurduğunu) fark ettim .

Bu sorunu çözmek için:


Donanım Tercihim: Çift çekirdekli mimarisi nedeniyle ESP32'yi seçtim.


Yazılım Mimarisi: İşlemci gücünü tam verimle kullanmak adına FreeRTOS işletim sistemini projeme entegre ettim .

Böylece; sensör okuma, ekran güncelleme ve buton tepkilerini birbirinden bağımsız Görevler (Tasks) olarak kurguladım. Sonuç olarak, arayüzde hiçbir donma yaşamadan gerçek zamanlı (Real-Time) çalışan bir sistem ortaya çıkardım.
