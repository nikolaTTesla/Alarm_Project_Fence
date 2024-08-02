package com.example.orchard_alarm;

import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;
import android.text.format.DateFormat;
import android.util.Log;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;

import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.database.ValueEventListener;

import java.util.Calendar;

public class MainActivity extends AppCompatActivity {

    private static final String CHANNEL_ID = "orchard_alarm_channel";
    private static final String GROUP_KEY = "com.example.orchard_alarm.NOTIFICATIONS";
    private static final String TAG = "MainActivity";
    private TextView textView;
    private int notificationId = 0;
    private String latestAlertMessage = "";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        textView = findViewById(R.id.textID);

        // Create notification channel
        createNotificationChannel();

        // Start background service
        Intent serviceIntent = new Intent(this, BackgroundService.class);
        startService(serviceIntent);

        // Initialize Firebase Database
        FirebaseDatabase database = FirebaseDatabase.getInstance();
        DatabaseReference myRef = database.getReference("data");

        // Add a ValueEventListener to listen for changes in the database
        myRef.addValueEventListener(new ValueEventListener() {
            @Override
            public void onDataChange(@NonNull DataSnapshot snapshot) {
                // Access the nested "Alert" data
                String alertMessage = snapshot.child("Alert").getValue(String.class);
                if (alertMessage != null && !alertMessage.isEmpty()) {
                    latestAlertMessage = alertMessage;
                    textView.setText(alertMessage);
                    sendNotification(alertMessage);
                }
            }

            @Override
            public void onCancelled(@NonNull DatabaseError error) {
                // Handle possible errors
                Log.e(TAG, "Database error: " + error.getMessage());
            }
        });

        // Register BootReceiver to restart service on device boot
        IntentFilter filter = new IntentFilter(Intent.ACTION_BOOT_COMPLETED);
        registerReceiver(new BootReceiver(), filter);
    }

    private void createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            CharSequence name = "Orchard Alarm Channel";
            String description = "Channel for orchard alarm notifications";
            int importance = NotificationManager.IMPORTANCE_HIGH;  // Set importance to high for heads-up notifications
            NotificationChannel channel = new NotificationChannel(CHANNEL_ID, name, importance);
            channel.setDescription(description);
            channel.enableLights(true);
            channel.setLightColor(NotificationCompat.COLOR_DEFAULT);
            channel.enableVibration(true);

            NotificationManager notificationManager = getSystemService(NotificationManager.class);
            if (notificationManager != null) {
                notificationManager.createNotificationChannel(channel);
            }
        }
    }

    private String getCurrentTimeStamp() {
        Calendar calendar = Calendar.getInstance();
        return DateFormat.format("MM-dd-yyyy HH:mm:ss", calendar).toString();
    }

    private void sendNotification(String messageBody) {
        String timestamp = getCurrentTimeStamp();
        String notificationText = messageBody + " - " + timestamp;

        Intent intent = new Intent(this, MainActivity.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, intent, PendingIntent.FLAG_ONE_SHOT | PendingIntent.FLAG_IMMUTABLE);

        NotificationCompat.Builder notificationBuilder = new NotificationCompat.Builder(this, CHANNEL_ID)
                .setSmallIcon(R.drawable.ic_notification) // Ensure this icon exists
                .setContentTitle("Orchard Alarm")
                .setContentText(notificationText)
                .setAutoCancel(true)
                .setContentIntent(pendingIntent)
                .setPriority(NotificationCompat.PRIORITY_HIGH)  // Set priority to high for heads-up notifications
                .setDefaults(NotificationCompat.DEFAULT_ALL)
                .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
                .setCategory(NotificationCompat.CATEGORY_ALARM)
                .setGroup(GROUP_KEY); // Set the group key for stacking

        NotificationManagerCompat notificationManager = NotificationManagerCompat.from(this);
        if (ActivityCompat.checkSelfPermission(this, android.Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) {
            // TODO: Consider calling
            //    ActivityCompat#requestPermissions
            // here to request the missing permissions, and then overriding
            //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
            //                                          int[] grantResults)
            // to handle the case where the user grants the permission. See the documentation
            // for ActivityCompat#requestPermissions for more details.
            return;
        }
        notificationManager.notify(notificationId++, notificationBuilder.build());

        // Create or update the summary notification that will show the stacked notifications
        NotificationCompat.Builder summaryNotificationBuilder = new NotificationCompat.Builder(this, CHANNEL_ID)
                .setSmallIcon(R.drawable.ic_notification)
                .setContentTitle("Orchard Alarm")
                .setContentText("Latest alert: " + latestAlertMessage + " - " + timestamp) // Include timestamp in summary
                .setAutoCancel(true)
                .setPriority(NotificationCompat.PRIORITY_HIGH)
                .setDefaults(NotificationCompat.DEFAULT_ALL)
                .setGroup(GROUP_KEY)
                .setGroupSummary(true);

        notificationManager.notify(0, summaryNotificationBuilder.build());
    }

    public static class BackgroundService extends Service {

        private static final String CHANNEL_ID = "orchard_alarm_channel";
        private static final String GROUP_KEY = "com.example.orchard_alarm.NOTIFICATIONS";
        private static final String TAG = "BackgroundService";
        private int notificationId = 0;
        private String latestAlertMessage = "";

        @Override
        public void onCreate() {
            super.onCreate();
            createNotificationChannel();

            // Initialize Firebase Database
            FirebaseDatabase database = FirebaseDatabase.getInstance();
            DatabaseReference myRef = database.getReference("data");

            // Add a ValueEventListener to listen for changes in the database
            myRef.addValueEventListener(new ValueEventListener() {
                @Override
                public void onDataChange(@NonNull DataSnapshot snapshot) {
                    // Access the nested "Alert" data
                    String alertMessage = snapshot.child("Alert").getValue(String.class);
                    if (alertMessage != null && !alertMessage.isEmpty()) {
                        latestAlertMessage = alertMessage;
                        Log.d(TAG, "Alert received: " + alertMessage);
                        sendNotification(alertMessage);
                    }
                }

                @Override
                public void onCancelled(@NonNull DatabaseError error) {
                    // Handle possible errors
                    Log.e(TAG, "Database error: " + error.getMessage());
                }
            });
        }

        @Override
        public IBinder onBind(Intent intent) {
            return null; // We don't need to bind to this service
        }

        private void createNotificationChannel() {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                CharSequence name = "Orchard Alarm Channel";
                String description = "Channel for orchard alarm notifications";
                int importance = NotificationManager.IMPORTANCE_HIGH;  // Set importance to high for heads-up notifications
                NotificationChannel channel = new NotificationChannel(CHANNEL_ID, name, importance);
                channel.setDescription(description);
                channel.enableLights(true);
                channel.setLightColor(NotificationCompat.COLOR_DEFAULT);
                channel.enableVibration(true);

                NotificationManager notificationManager = getSystemService(NotificationManager.class);
                if (notificationManager != null) {
                    notificationManager.createNotificationChannel(channel);
                }
            }
        }

        private String getCurrentTimeStamp() {
            Calendar calendar = Calendar.getInstance();
            return DateFormat.format("MM-dd-yyyy HH:mm:ss", calendar).toString();
        }

        private void sendNotification(String messageBody) {
            String timestamp = getCurrentTimeStamp();
            String notificationText = messageBody + " - " + timestamp;

            Intent intent = new Intent(this, MainActivity.class);
            intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
            PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, intent, PendingIntent.FLAG_ONE_SHOT | PendingIntent.FLAG_IMMUTABLE);

            NotificationCompat.Builder notificationBuilder = new NotificationCompat.Builder(this, CHANNEL_ID)
                    .setSmallIcon(R.drawable.ic_notification) // Ensure this icon exists
                    .setContentTitle("Orchard Alarm")
                    .setContentText(notificationText)
                    .setAutoCancel(true)
                    .setContentIntent(pendingIntent)
                    .setPriority(NotificationCompat.PRIORITY_HIGH)  // Set priority to high for heads-up notifications
                    .setDefaults(NotificationCompat.DEFAULT_ALL)
                    .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
                    .setCategory(NotificationCompat.CATEGORY_ALARM)
                    .setGroup(GROUP_KEY); // Set the group key for stacking

            NotificationManagerCompat notificationManager = NotificationManagerCompat.from(this);
            if (ActivityCompat.checkSelfPermission(this, android.Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) {
                // TODO: Consider calling
                //    ActivityCompat#requestPermissions
                // here to request the missing permissions, and then overriding
                //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
                //                                          int[] grantResults)
                // to handle the case where the user grants the permission. See the documentation
                // for ActivityCompat#requestPermissions for more details.
                return;
            }
            notificationManager.notify(notificationId++, notificationBuilder.build());

            // Create or update the summary notification that will show the stacked notifications
            NotificationCompat.Builder summaryNotificationBuilder = new NotificationCompat.Builder(this, CHANNEL_ID)
                    .setSmallIcon(R.drawable.ic_notification)
                    .setContentTitle("Orchard Alarm")
                    .setContentText("Latest alert: " + latestAlertMessage + " - " + timestamp) // Include timestamp in summary
                    .setAutoCancel(true)
                    .setPriority(NotificationCompat.PRIORITY_HIGH)
                    .setDefaults(NotificationCompat.DEFAULT_ALL)
                    .setGroup(GROUP_KEY)
                    .setGroupSummary(true);

            notificationManager.notify(0, summaryNotificationBuilder.build());
        }
    }

    public static class BootReceiver extends BroadcastReceiver {

        @Override
        public void onReceive(Context context, Intent intent) {
            if (Intent.ACTION_BOOT_COMPLETED.equals(intent.getAction())) {
                Log.d(TAG, "Device booted. Starting BackgroundService.");
                Intent serviceIntent = new Intent(context, BackgroundService.class);
                context.startService(serviceIntent);
            }
        }
    }
}
