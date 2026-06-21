# Quran Foundation API

## Credentials

### Pre-Production (Test)

| Field | Value |
|---|---|
| Client ID | `07aa145f-ab4e-453f-95fa-65c75e2a7e0d` |
| Client Secret | `yyttJn5e-N7fpAPUCtnHm4rdos` |
| Auth Endpoint | `https://prelive-oauth2.quran.foundation` |
| API Base URL | `https://apis-prelive.quran.foundation` |

⚠️ Limited data, but all features enabled for testing.

### Production (Live)

| Field | Value |
|---|---|
| Client ID | `2ca5c4ac-4dad-4186-af9a-3f736614a241` |
| Client Secret | `cBvlkVG.cLfiJDFa47RVMj5I9z` |
| Auth Endpoint | `https://oauth2.quran.foundation` |
| API Base URL | `https://apis.quran.foundation` |

⚠️ Full Qur'ān content, but NO authentication / user features by default.

## Authentication

OAuth2 Client Credentials flow:

```
POST {auth-endpoint}/oauth2/token
Authorization: Basic {base64(client_id:client_secret)}
Content-Type: application/x-www-form-urlencoded

grant_type=client_credentials&scope=content
```

Response includes an `access_token` (lifetime: 3600s). Include it on every API request:

```
x-auth-token: {access_token}
x-client-id: {client_id}
```

## Surah Recitation (Audio)

Get per-ayah audio file URLs for a specific surah:

```
GET {api-base}/content/api/v4/chapters/{chapter_number}/recitations/{recitation_id}
Headers:
  x-auth-token: {access_token}
  x-client-id: {client_id}
```

**Path parameters:**
- `chapter_number`: 1–114
- `recitation_id`: ayah-by-ayah recitation ID from `/content/api/v4/resources/recitations`

**Query parameters (optional):**
- `page`: page number (default 1)
- `per_page`: records per call, max 50 (default 10)
- `fields`: comma-separated extra fields (`chapter_id`, `verse_number`, `verse_key`, `juz_number`, `hizb_number`, `rub_el_hizb_number`, `page_number`, `ruku_number`, `manzil_number`, `format`, `url`, `segments`, `duration`, `id`)

**Response:**
```json
{
  "audio_files": [
    {
      "verse_key": "1:1",
      "url": "Alafasy/mp3/001001.mp3"
    }
  ],
  "pagination": {
    "per_page": 10,
    "current_page": 1,
    "next_page": null,
    "total_pages": 1,
    "total_records": 7
  }
}
```

### Recitations (Resource)

List available reciters:

```
GET {api-base}/content/api/v4/resources/recitations
```

| ID | Reciter | Style |
|---|---|---|
| 7 | Mishari Rashid al-Afasy | Murattal |
| 1 | AbdulBaset AbdulSamad | Mujawwad |
| ... | | |

## Key Endpoints Summary

| Endpoint | Description |
|---|---|
| `POST /oauth2/token` | Get access token (client credentials) |
| `GET /content/api/v4/resources/recitations` | List available recitations |
| `GET /content/api/v4/chapters/{chapter}/recitations/{rec_id}` | Get audio files for a surah |
| `GET /content/api/v4/chapters` | List all chapters (surahs) |
| `GET /content/api/v4/verses/by_chapter/{chapter}` | Get verses for a chapter |
