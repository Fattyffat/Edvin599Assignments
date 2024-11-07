import spotipy
from spotipy.oauth2 import SpotifyOAuth
import time
import requests
import io
import sounddevice as sd
import librosa
import numpy as np

import serial
import time


hop_length = 512  # Standard hop length for librosa
current_color = [0, 0, 255]  # Initialize with blue (R=0, G=0, B=255)


# Set up Spotify credentials
SPOTIPY_CLIENT_ID = "cec0d9569694467d864d7f8266ab4ae6"
SPOTIPY_CLIENT_SECRET = "03c6e6156189477dab6040226ecc7db0"
SPOTIPY_REDIRECT_URI = "http://localhost:8888/callback"

# Spotify authorization
scope = "user-top-read"
sp = spotipy.Spotify(auth_manager=SpotifyOAuth(client_id=SPOTIPY_CLIENT_ID,
                                               client_secret=SPOTIPY_CLIENT_SECRET,
                                               redirect_uri=SPOTIPY_REDIRECT_URI,
                                               scope=scope))

# Demo songs list
demo_songs = [
   
    ("3yyWrz4CsVzkWPuxtZflov", "Take me Home Country Roads", "KSHMR"),
    # ("2SHnUyZq0zwmvRIl4WY77G", "Alive", "Krewella"),
    ("7FlHNJT4TC120CDvFOHzei", "Rewrite the Stars", ""),
    ("3LxG9HkMMFP0MZuiw3O2rF", "Good Things Fall Apart", "Illenium"),
    # ("1Hdh6lJUR1WZCiNPVWxLTA", "On Ten", "Arizona Zervas"),
    ("6jQPolTBq89z9AHrMrmYpS", "Super Human", "Slander"),
    ("0qwPUvS4Eofc19Uw2IDodG", "Harmony - Radio Edit", "Nicky Romero"),

]



# Initialize Arduino serial connection
try:
    arduino = serial.Serial('COM9', 38400)
    time.sleep(2)
except serial.SerialException as e:
    print(f"Error connecting to Arduino: {e}")
    arduino = None

# Fetch top 5 most-played songs
def get_top_songs(limit=5):
    results = sp.current_user_top_tracks(limit=limit, time_range='short_term')
    top_songs = [(track['id'], track['name'], track['artists'][0]['name']) for track in results['items']]
    return top_songs

# Get detailed audio features for each song
def get_song_features(song_id):
    features = sp.audio_features([song_id])[0]
    
    if features is None:
        print(f"Audio features unavailable for song ID: {song_id}")
        return None

    return {
        "danceability": features["danceability"],
        "energy": features["energy"],
        "tempo": features["tempo"],
        "beats": features["time_signature"]
    }



# send trigger laser function
def trigger_laser(duration_ms):
    if arduino is not None:
        try:
            # Send command byte followed by 16-bit duration
            duration_high = (duration_ms >> 8) & 0xFF
            duration_low = duration_ms & 0xFF
            arduino.write(b'L' + bytes([duration_high, duration_low]))
        except serial.SerialException as e:
            print(f"Error sending laser command: {e}")


def pump_water():
     if arduino is not None:
        try:
            arduino.write(b'C')
        except serial.SerialException as e:
            print(f"Error sending flash command: {e}")


def update_leds(current_frame, band_energies, flux, flux_threshold):
    # Calculate intensity from frequency bands
    intensity = max(band_energies[0][current_frame], band_energies[1][current_frame], band_energies[2][current_frame])

    # Use spectral flux to detect subtle changes in music
    if flux[current_frame] > flux_threshold or intensity > 0.3:  # React even to small changes
        # More dramatic hue transition based on spectral flux and frequency bands
        hue = (np.sin(current_frame * 0.1) + 1) * 180  # Faster hue transition for more drama
        hue += np.random.randint(-60, 60)  # Add more randomness for dynamic effects (wider range)

        # Make saturation and brightness more dynamic and vivid
        saturation = max(0.5, np.random.uniform(0.7, 1.0))  # Vary saturation between 70% to 100%
        value = max(0.6, intensity)  # Adjust brightness based on intensity

        # Convert HSV to RGB
        h = hue / 360
        i = int(h * 6)
        f = h * 6 - i
        p = value * (1 - saturation)
        q = value * (1 - f * saturation)
        t = value * (1 - (1 - f) * saturation)

        if i % 6 == 0:
            r, g, b = value, t, p
        elif i % 6 == 1:
            r, g, b = q, value, p
        elif i % 6 == 2:
            r, g, b = p, value, t
        elif i % 6 == 3:
            r, g, b = p, q, value
        elif i % 6 == 4:
            r, g, b = t, p, value
        else:
            r, g, b = value, p, q

        # Convert to 0-255 range and ensure minimum brightness for visibility
        r = int(r * 255)
        g = int(g * 255)
        b = int(b * 255)

        trigger_rgb(r, g, b)
        
# Function to send RGB values to Arduino
def trigger_rgb(r, g, b):
    global current_color
    if arduino is not None:
        try:
            # Clamp RGB values between 0 and 255
            r = max(0, min(255, r))
            g = max(0, min(255, g))
            b = max(0, min(255, b))

            current_color = [r, g, b]  # Update current_color when setting new RGB values
            
            # Send the clamped RGB values as bytes
            arduino.write(b'R' + bytes([r, g, b]))
        except serial.SerialException as e:
            print(f"Error sending RGB command: {e}")


def detect_music_events(y, sr):
    # Onset detection with smoothing
    onset_env = librosa.onset.onset_strength(y=y, sr=sr)
    onset_env_smooth = librosa.util.normalize(librosa.decompose.nn_filter(onset_env))
    onset_frames = librosa.onset.onset_detect(onset_envelope=onset_env_smooth, sr=sr)
    onset_times = librosa.frames_to_time(onset_frames, sr=sr)

    # Spectral flux with frequency bands
    spec = np.abs(librosa.stft(y))
    flux_low = np.sum(np.diff(spec[:len(spec)//3], axis=1), axis=0)
    flux_mid = np.sum(np.diff(spec[len(spec)//3:2*len(spec)//3], axis=1), axis=0)
    flux_high = np.sum(np.diff(spec[2*len(spec)//3:], axis=1), axis=0)

    # Combine all fluxes (or use separately for more control)
    flux_total = flux_low + flux_mid + flux_high
    flux_total = np.concatenate(([0], flux_total)) 

    # Beat tracking
    tempo, beat_frames = librosa.beat.beat_track(y=y, sr=sr)
    beat_times = librosa.frames_to_time(beat_frames, sr=sr)

    # Frequency band energies for LED control
    band_edges = [0, len(spec)//3, 2*len(spec)//3, len(spec)]
    frequency_bands = [spec[band_edges[i]:band_edges[i+1]] for i in range(3)]
    band_energies = [np.sum(band, axis=0) for band in frequency_bands]
    
    # Normalize energies
    band_energies = [librosa.util.normalize(energy) for energy in band_energies]

    return onset_times, flux_total, beat_times, band_energies


def play_song_preview(song_id, tempo):
    track = sp.track(song_id)
    preview_url = track['preview_url']
    
    if preview_url:
        try:
            # Fetch audio preview
            response = requests.get(preview_url)
            response.raise_for_status()
            audio_data = io.BytesIO(response.content)
            
            # Load audio data using librosa
            y, sr = librosa.load(audio_data, sr=None)
            
            # Detect music events (onsets, spectral flux, beats, band energies)
            onset_times, flux, beat_times, band_energies = detect_music_events(y, sr)
            
            # Set thresholds for detecting significant changes
            flux_threshold = np.mean(flux) + 2 * np.std(flux)
            major_flux_threshold = np.mean(flux) + 4 * np.std(flux)
            
            # Calculate beat interval based on tempo
            beat_interval = 60 / tempo
            laser_on_ratio = 0.25
            
            # Start playback with sounddevice
            sd.play(y, sr)
            start_time = time.time()
            
            # Initialize indices for tracking beats and flux events
            beat_index = 0
            flux_index = 0
            last_pump_time = 0
            pump_cooldown = 1.5
            
            while sd.get_stream().active:
                current_time = time.time() - start_time
                current_frame = int(current_time * sr / hop_length)  # Add hop_length constant
                
                # Beat detection (Laser activation)
                if beat_index < len(beat_times) and current_time >= beat_times[beat_index]:
                    next_beat_time = beat_times[beat_index + 1] if beat_index + 1 < len(beat_times) else beat_times[beat_index] + beat_interval
                    laser_duration = int((next_beat_time - beat_times[beat_index]) * laser_on_ratio * 1000)
                    trigger_laser(laser_duration)
                    beat_index += 1
                
                # Update LEDs based on frequency band energies
                if current_frame < len(band_energies[0]):
                    update_leds(current_frame, band_energies, flux, flux_threshold)
                
                # Spectral flux processing (for pump and major effects)
                if flux_index < len(flux) and current_time >= librosa.frames_to_time(flux_index, sr=sr):
                    if flux[flux_index] > major_flux_threshold:
                        if (current_time - last_pump_time) >= pump_cooldown:
                            pump_water()
                            last_pump_time = current_time
                    flux_index += 1
                
                time.sleep(0.01)  # Small delay to prevent CPU overuse
            
            sd.wait()  # Wait for playback to finish
        
        except requests.RequestException as e:
            print(f"Failed to download preview: {e}")
        except Exception as e:
            print(f"An unexpected error occurred: {e}")
    
    else:
        print(f"No preview available for '{track['name']}'")

def main():
    try: 
        # Uncomment one of the options below to switch between demo songs or top 5 songs
        print
        # Option 1: Use hardcoded demo songs
        songs = demo_songs
        print("Using demo songs for this session.")
        
        # Option 2: Fetch top 5 most-played songs
        # songs = get_top_songs()
        # print("Fetching top 5 songs...")

        print("\nSongs:")
        for index, (song_id, song_name, artist_name) in enumerate(songs, start=1):
            print(f"{index}. {song_name} by {artist_name}")

        for song_id, song_name, artist_name in songs:
            print(f"\nNow playing: {song_name} by {artist_name}")
            
            # Get song features and check if they are available
            features = get_song_features(song_id)
            if features is None:
                print(f"Skipping '{song_name}' due to missing audio features.")
                continue

            print(f"Song features: {features}")

            # Play the song preview and simulate beats based on tempo
            play_song_preview(song_id, features["tempo"])

            time.sleep(1)  # Short delay before next song

    finally: 
        if arduino is not None:
            
            arduino.close()
            print("Arduino connection closed")

if __name__ == "__main__":
    main()