// Circular buffer type for ADC averaging filter
// N: Filter order
// Type: type of data

template <int N, class Type>
class LowPassBuffer {
  public:
    void Init(void) {
      m_data_len = 0;
      m_current_pointer = 0;
    }

    void AddData(Type x) {
      m_buffer[m_current_pointer] = x;

      m_current_pointer++;
      if (m_current_pointer >= N) {
        m_current_pointer = 0;
      }

      m_data_len++;
      if (m_data_len > N) {
        m_data_len = N;
      }
    }

    Type GetAverage() {
      Type sum = 0;
      for (uint8_t i=0; i<m_data_len;i++) {
        sum += m_buffer[i];
      }

      return (sum / m_data_len);
    }
        
  private:
    Type m_buffer[N];
    uint8_t m_data_len;
    uint8_t m_current_pointer;
};
